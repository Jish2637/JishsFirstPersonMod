"""Offline bridge inspection. Reads PE files; never loads DLLs or edits profiles."""
import struct
import argparse
import bisect
import hashlib
import pathlib
import re
import sys

import pefile
from capstone import Cs, CS_ARCH_X86, CS_MODE_64
from capstone.x86 import X86_OP_IMM, X86_OP_MEM, X86_REG_RIP

FIELDS = ('status_rva', 'state_pointer_rva', 'mutex_lock_rva', 'mutex_unlock_rva',
          'write_fov_rva', 'source_initialized_rva', 'source_base_rva', 'source_transform_rva')

class InspectionError(Exception):
    pass

def unique(values, label):
    values = sorted(set(values))
    if len(values) != 1:
        raise InspectionError(f'{label}: expected one match, found {len(values)}: ' +
                              ', '.join(hex(v) for v in values))
    return values[0]

def profiles(path):
    pattern = r'\{"([^"]+)","([0-9a-f]{64})",\s*([^}]+)\}'
    result = {}
    for version, digest, numbers in re.findall(pattern, path.read_text()):
        addresses = [int(s.strip(), 0) for s in numbers.split(',')]
        if len(addresses) != len(FIELDS):
            raise InspectionError('NativeContract schema changed; update the inspection tool')
        result[digest] = (version, dict(zip(FIELDS, addresses)))
    return result

class Image:
    def __init__(self, path):
        self.digest = hashlib.sha256(path.read_bytes()).hexdigest()
        self.pe = pefile.PE(str(path), fast_load=True)
        self.pe.parse_data_directories(directories=[pefile.DIRECTORY_ENTRY["IMAGE_DIRECTORY_ENTRY_EXCEPTION"], pefile.DIRECTORY_ENTRY["IMAGE_DIRECTORY_ENTRY_EXPORT"]])
        if self.pe.FILE_HEADER.Machine != 0x8664:
            raise InspectionError('Expected a Windows x64 DLL')
        self.md = Cs(CS_ARCH_X86, CS_MODE_64)
        self.md.detail = True
        if not hasattr(self.pe,'DIRECTORY_ENTRY_EXCEPTION'):
            raise InspectionError('DLL has no x64 unwind directory')
        self.functions = sorted((e.struct.BeginAddress, e.struct.EndAddress)
                                for e in self.pe.DIRECTORY_ENTRY_EXCEPTION)
        self.starts = [a for a, _ in self.functions]

    def code(self, rva, size):
        return list(self.md.disasm(self.pe.get_data(rva, size), rva))

    def function(self, rva):
        index = bisect.bisect_right(self.starts, rva) - 1
        if index < 0 or not self.functions[index][0] <= rva < self.functions[index][1]:
            raise InspectionError(f'No unwind function contains {rva:#x}')
        return self.functions[index]

    def section(self, rva, size, flags):
        return any(s.VirtualAddress <= rva and
                   rva + size <= s.VirtualAddress + s.Misc_VirtualSize and
                   s.Characteristics & flags == flags for s in self.pe.sections)

    def export(self, name):
        symbols = getattr(getattr(self.pe,'DIRECTORY_ENTRY_EXPORT',None),'symbols',[])
        return unique([s.address for s in symbols
                       if s.name == name.encode()], name)

def rip(i):
    return any(o.type == X86_OP_MEM and o.mem.base == X86_REG_RIP for o in i.operands)

def destination(i):
    return i.address + i.size + i.disp

def calls(code):
    result = []
    for i in code:
        if i.mnemonic == 'ret':
            break
        if i.mnemonic == 'call' and i.operands[0].type == X86_OP_IMM:
            result.append(i.operands[0].imm)
    return result

def signature(old, new, rva, limit=96, layout_replacements=None):
    # MSVC unwind ranges can describe only a prologue or split function fragment.
    # Decode a complete bounded prefix rather than treating .pdata EndAddress as a return.
    instructions = old.code(rva, limit)
    if not instructions:
        raise InspectionError(f'Cannot decode baseline function {rva:#x}')
    size = instructions[-1].address + instructions[-1].size - rva
    data = bytearray(old.pe.get_data(rva, size))
    for offset, (before, after) in (layout_replacements or {}).items():
        if struct.unpack_from("<i", data, offset)[0] != before:
            raise InspectionError("Reviewed status layout operand differs")
        struct.pack_into("<i", data, offset, after)
    wild = set()
    for i in instructions:
        if rip(i):
            wild.update(range(i.address-rva+i.disp_offset, i.address-rva+i.disp_offset+i.disp_size))
        # Mask relative branches, but preserve constant ABI/layout operands.
        if i.mnemonic.startswith('j') or i.mnemonic == 'call' or (limit==160 and i.address-rva==0x67 and i.mnemonic=='mov' and i.op_str.startswith('ecx,')):
            wild.update(range(i.address-rva+i.imm_offset, i.address-rva+i.imm_offset+i.imm_size))
    pattern = b''.join(b'.' if j in wild else re.escape(bytes([b])) for j, b in enumerate(data))
    matches = []
    for sec in new.pe.sections:
        if sec.Characteristics & 0x20000000:
            matches.extend(sec.VirtualAddress + m.start()
                           for m in re.finditer(pattern, sec.get_data(), re.DOTALL))
    return unique(matches, f'function signature {rva:#x}')

def source_initializer(image):
    # Proven game camera setter reference, not a ReSkate RVA or version-specific shift.
    pattern = bytes.fromhex('488d8790faf802')
    candidates = []
    for sec in image.pe.sections:
        if sec.Characteristics & 0x20000000:
            data = sec.get_data()
            for match in re.finditer(re.escape(pattern), data):
                start, end = image.function(sec.VirtualAddress + match.start())
                code = image.code(start, max(end-start,256))
                stores = [i for i in code if i.mnemonic == 'mov' and rip(i)
                          and i.operands[0].type == X86_OP_MEM]
                if any(i.op_str.endswith(', 1') and i.operands[0].size == 1 for i in stores):
                    candidates.append(start)
    return unique(candidates, 'SourceTrial initializer')

def source_blocks(root):
    specifications = {
        'Engine/Game/Skater/first_person_spring.h': [('struct', 'Settings')],
        'Extension/UI/Overlay/overlay.h': [('enum class', 'DebugAction'), ('struct', 'DebugRequest'),
                                         ('struct', 'DebugModel'), ('struct', 'Model'), ('struct', 'CallbacksV3')],
        'Extension/Skater/client_source_spawn_internal.h': [('struct', 'SourceTrial'), ('struct', 'SourceState')],
        'Extension/UI/Overlay/overlay_internal.h': [('struct', 'State')],
    }
    result = {}
    for file, declarations in specifications.items():
        text = (root/file).read_text(encoding='utf-8-sig')
        text = re.sub(r'/\*.*?\*/|//[^\n]*', '', text, flags=re.S)
        for kind, name in declarations:
            match = re.search(r'\b'+kind+r'\s+'+name+r'\s*\{', text)
            if not match:
                raise InspectionError(f'Source declaration missing: {file}: {name}')
            start = match.end()-1
            depth = 1
            end = start+1
            while depth and end < len(text):
                depth += (text[end] == '{') - (text[end] == '}')
                end += 1
            if depth:
                raise InspectionError(f'Unclosed source declaration: {name}')
            block = text[start:end]
            if name == 'State':
                # Fields after Model do not affect the bridge's prefix offsets.
                marker = 'dingosdk::overlay::Model model;'
                if marker not in block:
                    raise InspectionError('Overlay State model field changed')
                block = block[:block.index(marker)+len(marker)]
            result[file+':'+name] = re.sub(r'\s+', '', block)
    game_header = 'Engine/Game/Build/20260929/client_source_spawn.h'
    result[game_header] = re.sub(r'\s+', '', (root/game_header).read_text())
    return result

def inspect(old, new, baseline, callbacks_offset=0x4b0, model_offset=0x4e0, new_callbacks_offset=None, new_model_offset=None):
    result = {}
    status = new.export('DingoSDKOverlayGetStatus')
    layout = {0x3a:(0x320,0x328),0x49:(0x321,0x329),0x50:(0x428,0x430),0x6d:(0x2b4,0x2bb),0x79:(0x328,0x330),0x85:(0x478,0x490)} if new_callbacks_offset==0x4d0 and callbacks_offset==0x4b8 else None
    if new_callbacks_offset == 0x590 and callbacks_offset == 0x4d0:
        layout = {0x50:(0x430,0x4f0),0x85:(0x490,0x550)}
    expected_status = signature(old, new, baseline['status_rva'],166, layout_replacements=layout)
    if status != expected_status:
        raise InspectionError('Status export no longer matches verified implementation')
    result['status_rva'] = status
    status_calls = calls(new.code(status,256))
    old_calls = calls(old.code(baseline['status_rva'],256))
    if len(status_calls) != 3 or len(old_calls) != 3:
        raise InspectionError('Status accessor/mutex call structure changed')
    accessor, lock, unlock = status_calls
    old_lock = old.code(baseline['mutex_lock_rva'],7)
    new_lock = new.code(lock,7)
    if len(old_lock)!=2 or len(new_lock)!=2 or old_lock[0].op_str!='edx, edx' or new_lock[0].op_str!='edx, edx' or old_lock[1].mnemonic!='jmp' or new_lock[1].mnemonic!='jmp':
        raise InspectionError('Mutex lock thunk ABI changed')
    if new_lock[1].operands[0].imm != signature(old,new,old_lock[1].operands[0].imm) or unlock != signature(old,new,baseline['mutex_unlock_rva']):
        raise InspectionError('Status mutex functions differ from matched ABI')
    tls_layout = {0x14:(0xe8b8,0xead0)} if old.digest=='8ae11491fca59317e561211e6abffb0e0dc26890a8845c8e1b0217c2c1a0a427' and new.digest=='ef237e13b2c5ade4e24a7bbdd7824680cb79fd6bdf8d1c13340a0d375ddd2fda' else {0x14:(0xe8a8,0xe8b8)} if new_callbacks_offset==0x4d0 and callbacks_offset==0x4b8 else None
    if accessor != signature(old,new,old_calls[0],160,tls_layout):
        raise InspectionError('State accessor changed')
    def constructor(image, getter, cb_offset, mdl_offset):
        candidates = []
        for i in image.code(getter,160):
            if i.mnemonic!='call' or i.operands[0].type!=X86_OP_IMM:
                continue
            address=i.operands[0].imm
            body=image.code(address,0x800)
            if any(j.op_str==f'qword ptr [rdi + {cb_offset:#x}], rsi' for j in body) and any(j.op_str==f'rcx, [rdi + {mdl_offset:#x}]' for j in body):
                candidates.append(address)
        return unique(candidates,'overlay constructor with callback/model layout')
    if constructor(new,accessor,new_callbacks_offset or callbacks_offset,new_model_offset or model_offset)!=signature(old,new,constructor(old,old_calls[0],callbacks_offset,model_offset)):
        raise InspectionError('Overlay constructor/layout changed')
    result['mutex_lock_rva'], result['mutex_unlock_rva'] = lock, unlock
    state_reads = [destination(i) for i in new.code(accessor, 0x33)
                   if i.mnemonic == 'mov' and i.op_str.startswith('rax, qword ptr [rip')]
    result['state_pointer_rva'] = unique(state_reads, 'state pointer')
    result['write_fov_rva'] = signature(old,new,baseline['write_fov_rva'])
    fov = new.code(result['write_fov_rva'], 160)
    if not any(i.op_str == 'dword ptr [rbx + 0xac], xmm6' for i in fov):
        raise InspectionError('FOV writer no longer writes the verified camera field')
    old_init = source_initializer(old)
    init = source_initializer(new)
    init_tls = {0x29:(0xe8b8,0xead0)} if tls_layout=={0x14:(0xe8b8,0xead0)} else {0x29:(0xe8a8,0xe8b8)} if new_callbacks_offset==0x4d0 and callbacks_offset==0x4b8 else None
    if signature(old,new,old_init,layout_replacements=init_tls) != init:
        raise InspectionError('SourceTrial initializer changed')
    old_code = old.code(old_init,max(old.function(old_init)[1]-old_init,256))
    new_code = new.code(init,max(new.function(init)[1]-init,256))
    for name in FIELDS[5:]:
        refs = [i for i in old_code if rip(i) and destination(i) == baseline[name]]
        mapped = []
        for ref in refs:
            offset = ref.address-old_init
            for candidate in new_code:
                if candidate.address-init == offset and rip(candidate) and candidate.mnemonic == ref.mnemonic:
                    mapped.append(destination(candidate))
        result[name] = unique(mapped,name)
    for name in FIELDS:
        flags = 0x20000000 if name in ('status_rva','mutex_lock_rva','mutex_unlock_rva','write_fov_rva') else 0xC0000000
        width = 1 if name == 'source_initialized_rva' else 8
        if not new.section(result[name],width,flags):
            raise InspectionError(f'{name} is outside the expected PE section')
    if len(set(result.values())) != len(result):
        raise InspectionError('Resolved bridge addresses overlap')
    return result

def entry(version, digest, result):
    return ('{"'+version+'","'+digest+'",\n '+
            ','.join(hex(result[name]) for name in FIELDS)+'},')

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--dll',type=pathlib.Path,required=True)
    parser.add_argument('--baseline',type=pathlib.Path,required=True)
    parser.add_argument('--version',required=True)
    parser.add_argument('--source',type=pathlib.Path,required=True)
    parser.add_argument('--baseline-source',type=pathlib.Path,required=True)
    parser.add_argument('--contracts',type=pathlib.Path,default=pathlib.Path(__file__).resolve().parents[1]/'src/stock_contract.h')
    args = parser.parse_args()
    try:
        if not re.fullmatch(r'\d+\.\d+\.\d+(?:[-.][A-Za-z0-9]+)*',args.version):
            raise InspectionError('Invalid version label')
        old,new = Image(args.baseline),Image(args.dll)
        known = profiles(args.contracts)
        if old.digest not in known:
            raise InspectionError('Baseline DLL is not a verified compiled profile')
        old_version,baseline = known[old.digest]
        if old.export('DingoSDKOverlayGetStatus') != baseline['status_rva']:
            raise InspectionError('Baseline export differs from compiled profile')
        a,b = source_blocks(args.baseline_source),source_blocks(args.source)
        if args.version == "2.0.8" and old_version == "2.0.7":
            key='Extension/UI/Overlay/overlay.h:Model'
            addition='GrindExitModelgrind_exit;'
            if addition not in b[key]: raise InspectionError('Reviewed grind exit model addition missing')
            b[key]=b[key].replace(addition,'',1)
        if args.version == "2.0.6" and old_version == "2.0.5":
            additions = {
                'Engine/Game/Skater/first_person_spring.h:Settings': ['boolcamera_cycle=false;'],
                'Extension/UI/Overlay/overlay.h:DebugAction': ['set_first_person_camera_cycle,'],
                'Extension/UI/Overlay/overlay.h:Model': ['BoardBreakModelboard_break;'],
                'Extension/UI/Overlay/overlay_internal.h:State': [
                    'staticconstexprstd::size_tmore_swapchains=3;',
                    'std::array<Hook,more_swapchains>present_more,present1_more,resize_more,resize1_more;'],
            }
            for key, edits in additions.items():
                for edit in edits:
                    if b[key].count(edit) != 1:
                        raise InspectionError('Reviewed 2.0.6 addition missing: '+edit)
                    b[key] = b[key].replace(edit, '', 1)
        # Reviewed 2.0.3 additions: RoadRash follows the bridge's model prefix;
        # State gains input flags and two resource pointers before callbacks.
        # Require precisely these source edits; arbitrary layout changes still fail.
        if args.version == "2.0.3" and old_version == "2.0.2":
            model_key = 'Extension/UI/Overlay/overlay.h:Model'
            state_key = 'Extension/UI/Overlay/overlay_internal.h:State'
            if b[model_key].count('RoadRashModelroad_rash;') != 1:
                raise InspectionError('Reviewed RoadRash model addition missing')
            b[model_key] = b[model_key].replace('RoadRashModelroad_rash;', '', 1)
            additions = [
                'std::atomic<bool>hub_pointer{false},hub_typing{false};',
                'std::atomic<bool>prompt_pointer{false};',
                'std::atomic<bool>item_search_visible{false},item_search_focus_requested{false};',
                'std::atomic<bool>item_search_character_pending{false},item_search_escape_pending{false};',
                'std::atomic<bool>prompt_input_active{false};',
                'ComPtr<ID3D12Resource>card_texture,card_upload;',
            ]
            for addition in additions:
                if b[state_key].count(addition) != 1:
                    raise InspectionError('Reviewed 2.0.3 State addition missing: '+addition)
                b[state_key] = b[state_key].replace(addition, '', 1)
        changed = [key for key in a if a[key] != b[key]]
        if changed:
            raise InspectionError('Source layout/API changed; investigate: '+', '.join(changed))
        result = inspect(old,new,baseline,0x590 if old_version in ("2.0.6", "2.0.7", "2.0.8") else 0x4d0 if old_version in ("2.0.3", "2.0.4", "2.0.5") else 0x4b8 if old_version.startswith("2.") else 0x4b0,
                         0x5c0 if old_version in ("2.0.6", "2.0.7", "2.0.8") else 0x500 if old_version in ("2.0.3", "2.0.4", "2.0.5") else 0x4e8 if old_version.startswith("2.") else 0x4e0,
                         0x590 if args.version in ("2.0.6", "2.0.7", "2.0.8") else 0x4d0 if args.version in ("2.0.3", "2.0.4", "2.0.5") else None,
                         0x5c0 if args.version in ("2.0.6", "2.0.7", "2.0.8") else 0x500 if args.version in ("2.0.3", "2.0.4", "2.0.5") else None)
        print(f'{old_version} -> {args.version}\nSHA-256: {new.digest}')
        for name in FIELDS:
            print(f'{name:24} {baseline[name]:#010x} -> {result[name]:#010x}  OK')
        if new.digest in known and result != known[new.digest][1]:
            raise InspectionError('Discovered addresses disagree with existing verified profile')
        print('Source layouts, unique binary matches and PE sections: OK')
        print('\nNativeContract entry (paste into src/stock_contract.h):\n'+entry(args.version,new.digest,result))
        return 0
    except (InspectionError,OSError,ValueError,pefile.PEFormatError) as error:
        print(f'BLOCKED: {error}\nNo completed profile entry emitted.',file=sys.stderr)
        return 1

if __name__ == '__main__':
    sys.exit(main())
