"""Offline audit of every private 2.1.4-5 runtime interface. Never loads the DLL.
Usage: python tools/audit_214_5.py DLL --upstream TAG_SOURCE
Dependencies: tools/requirements.txt. Compile tools/layout214_5 separately first.
"""
import argparse,hashlib,json,pathlib,re,struct,sys
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT.parent/'.trace-deps'))
from inspect_hooks import Image,calls,destination,rip,signature
p=argparse.ArgumentParser();p.add_argument('dll',type=pathlib.Path);p.add_argument('--upstream',type=pathlib.Path,required=True);a=p.parse_args()
im=Image(a.dll);assert im.digest=='0820fa71c1cb716f4222eb9027130933aa9e3db4ce20b46ef555760be997dbde','DLL identity'
folder=ROOT/'tools/migration214_5';entries=json.loads((folder/'contracts.json').read_text());slots=json.loads((folder/'data-contracts.json').read_text())
def address(role):return entries[role]['rva']
def relative(at,disp,size):return at+size+struct.unpack('<i',im.pe.get_data(at+disp,4))[0]
for role,e in entries.items():
 data=bytes.fromhex(e['prefix']);assert im.section(e['rva'],len(data),0x20000000),role
 assert im.pe.get_data(e['rva'],len(data))==data,role
assert im.export('DingoSDKOverlayGetStatus')==address('status')
assert calls(im.code(address('status'),180))==[address('getter'),address('lock'),address('unlock')],'status/mutex relationship'
assert im.pe.get_data(address('lock'),3)==b'\x33\xd2\xe9','mutex lock ABI'
assert relative(address('lock')+2,1,5)==address('lock_impl'),'mutex thunk destination'
previous=Image(ROOT.parent/'upstream/ReSkate-2.1.4-4.dll')
assert signature(previous,im,entries['lock_impl']['old'],96)==address('lock_impl'),'recursive mutex implementation ABI'
for key in ('state','party','remote','session','context','alloc','free','userdata','source_initialized','source_base','source_transform'):
 assert im.section(slots[key],8,0xC0000000),key+' writable global'
for slot,role in [('state','getter'),('party','party_getter'),('remote','remote_getter'),('session','msc_session')]:
 assert slots[slot] in {destination(i) for i in im.code(address(role),100) if rip(i)},slot+' accessor'
assert relative(address('getter')+0x27,3,7)==slots['state']
ctor=im.code(address('ctor'),2300)
assert any(i.op_str=='qword ptr [rdi + 0xac8], rsi' for i in ctor),'callback offset'
assert any(i.op_str=='rcx, [rdi + 0xaf8]' for i in ctor),'model offset'
for role in ('mem_alloc','mem_free'):
 refs={destination(i) for i in im.code(address(role),170) if rip(i)}
 assert {slots['context'],slots['userdata'],slots['free' if role=='mem_free' else 'alloc']}<=refs,role
assert destination(im.code(address('context'),7)[0])==slots['context']
code=im.code(address('source_init'),180)
assert any('0x2f8fa90' in i.op_str for i in code),'game camera transform publication'
for key in ('source_initialized','source_base','source_transform'):
 assert slots[key] in {destination(i) for i in code if rip(i)},key
assert relative(slots['party_fov_return']-5,1,5)==address('fov'),'party FOV caller'
assert im.pe.get_data(slots['party_transform_return']-2,2)==b'\xff\xd0','party transform ABI'
for site in slots['hid_callers']:assert relative(site,1,5)==address('hid'),'HID caller'
assert len(slots['hid_callers'])==3
pad=im.code(address('gamepad'),0x400)
assert any(i.mnemonic=='movups' and i.op_str=='xmmword ptr [r14], xmm0' for i in pad),'native 16-byte pad copy'
assert any(i.mnemonic=='mov' and i.op_str=='dword ptr [r14 + 0x10], eax' for i in pad),'native pad final 4-byte copy'
# Validate the guards and literals actually used by the shipping sources.
for header in ('native_211.h','embedded_contract.h'):
 text=(ROOT/'src'/header).read_text()
 for role,rva,prefix in re.findall(r'Entry\s+\w+\{"([^"]+)",\s*(0x[0-9a-f]+),\s*"([0-9a-f]+)"',text):
  assert int(rva,16)==address(role) and prefix==entries[role]['prefix'],(header,role)
text=(ROOT/'src/msc_presence_bridge.cpp').read_text()
for rva,values in re.findall(r'bytes\(base\+(0x[0-9a-f]+),std::array<unsigned char,\d+>\{([^}]+)}',text):
 data=bytes(int(v.strip(),0) for v in values.split(','));assert im.pe.get_data(int(rva,16),len(data))==data,'MSC guard'
code=im.code(address('msc_send'),100);assert {784664,784720}<={o.mem.disp for i in code for o in i.operands if o.type==3},'MSC fields'
text=(ROOT/'src/stock_hid_contract.h').read_text()
prefix=bytes(int(x,0) for x in re.search(r'stock_hid_prefix\{([^}]+)',text)[1].split(','))
assert im.pe.get_data(address('hid'),len(prefix))==prefix,'HID guard'
text=(ROOT/'src/spectator_contract.h').read_text()
for label,rva in [('party',address('party_getter')),('remote',address('remote_getter')),('render',address('render')),('transform',slots['party_transform_return']-17)]:
 data=bytes.fromhex(re.search(label+r'_signature="([0-9a-f]+)',text)[1]);assert im.pe.get_data(rva,len(data))==data,'spectator '+label
page=im.code(address('draw'),0xd00)
strings={im.pe.get_data(destination(i),96).split(b'\0')[0] for i in page if i.mnemonic=='lea' and rip(i)}
assert {b'first-person',b'FIRST PERSON',b'Field of view',b'TRUE FIRST PERSON'}<=strings,'page identity'
assert {address(n) for n in ('begin_card','end_card','field','note','slider_float','toggle_row')}<=set(calls(page)),'widget call graph'
assert address('draw') in calls(im.code(address('skater_page'),0x200)),'page dispatch'
# Compare the compiled Release layout against the previous official tag.
def layout(path):return dict(line.split('=',1) for line in path.read_text(encoding='utf-8-sig').splitlines() if '=' in line)
baseline=ROOT/'tools/migration214_4'
before=layout(baseline/'layout.txt');after=layout(folder/'layout.txt')
assert before==after,'compiled field offsets or sizes changed'
assert len(entries)==33 and len(after)==68
prior_entries=json.loads((baseline/'contracts.json').read_text())
assert entries.keys()==prior_entries.keys()
for role,entry in entries.items():
 assert entry['rva']==prior_entries[role]['rva'] and entry['prefix']==prior_entries[role]['prefix'],role
assert slots==json.loads((baseline/'data-contracts.json').read_text()),'globals or call sites changed'
prior_code=next(section for section in previous.pe.sections if section.Name.rstrip(b'\0')==b'.text')
code_section=next(section for section in im.pe.sections if section.Name.rstrip(b'\0')==b'.text')
assert code_section.VirtualAddress==prior_code.VirtualAddress and code_section.get_data()==prior_code.get_data(),'native code differs'
assert after['M.menu_layout']=='24508'
assert after['Mutex.size']==after['RecursiveMutex.size']=='80','MSVC mutex representation'
assert after['R.mutex']=='72','remote mutex position'
assert after['M.revisions']=='26216' and after['Model.size']=='26248','new partial-model revisions'
assert 'debug_revision_offset=26224' in (ROOT/'src/native_211.h').read_text(),'compiled debug revision offset'
read_body=im.code(address('read'),0x90)
assert any(i.op_str=='xmm6, xmmword ptr [rsi + 0x6668]' for i in read_body),'native model revision load'
assert any(i.op_str=='xmm7, xmmword ptr [rsi + 0x6678]' for i in read_body),'native remaining revisions load'
assert any(i.mnemonic=='mov' and i.op_str=='ecx, 0xc0538' for i in im.code(address('msc_session'),160)),'native Session allocation size'
assert any(i.mnemonic=='mov' and i.op_str=='ecx, 0xbe40' for i in im.code(address('party_getter'),160)),'native Party allocation size'
assert any(i.mnemonic=='imul' and i.op_str.endswith(', 0x390') for i in im.code(address('remote_getter'),100)),'remote stride in native accessor'
text=(ROOT/'src/spectator_contract.h').read_text()
assert 'remote_stride=912' in text,'compiled spectator stride'
text=(ROOT/'src/embedded_contract.h').read_text()
assert 'state_context=2752,state_menu=29064,state_window=728' in text,'compiled overlay offsets'
text=(ROOT/'src/stock_contract.h').read_text()
assert '? 0xac8:' in text and '? 0xaf8:' in text,'compiled callbacks/model offsets'
profile=re.search(r'\{"2\.1\.4-5","([0-9a-f]{64})",\s*([^}]+)}',text)
assert profile and profile[1]==im.digest,'compiled native identity'
assert [int(v.strip(),0) for v in profile[2].split(',')]==[
 address('status'),slots['state'],address('lock'),address('unlock'),address('fov'),
 slots['source_initialized'],slots['source_base'],slots['source_transform']],'compiled native globals'
def constants(text):return {k:int(v,16) for k,v in re.findall(r'\b(\w+)=(0x[0-9a-f]+)',text)}
ui=constants((ROOT/'src/embedded_contract.h').read_text())
for field,slot in [('context_slot','context'),('alloc_slot','alloc'),('free_slot','free'),('userdata_slot','userdata')]:assert ui[field]==slots[slot],field
watch=constants((ROOT/'src/spectator_contract.h').read_text())
for field,expected in {'party_getter':address('party_getter'),'remote_getter':address('remote_getter'),
 'party_pointer':slots['party'],'remote_pointer':slots['remote'],'render_hook':address('render'),
 'party_transform_return':slots['party_transform_return'],'party_fov_return':slots['party_fov_return']}.items():assert watch[field]==expected,field
hid=(ROOT/'src/stock_hid_contract.h').read_text()
assert [int(v,0) for v in re.search(r'stock_hid_callers\{([^}]+)',hid)[1].split(',')]==slots['hid_callers'],'compiled HID callers'
msc=(ROOT/'src/msc_presence_bridge.cpp').read_text()
assert 'session+787688' in msc and 'session+785352+882' in msc,'compiled session mutex/view/connected offsets'
assert 'session+783584' not in msc and 'session+781352' not in msc,'obsolete session offsets retained'
for value in (slots['session'],address('msc_command'),address('msc_queue'),address('msc_send')):assert hex(value) in msc,'compiled MSC interface'
assert relative(address('msc_session')+0x27,3,7)==slots['session'],'MSC session accessor'
sources=['Extension/UI/Overlay/overlay_internal.h','Extension/UI/Overlay/overlay.h','Extension/UI/Overlay/input_capture.cpp','Extension/UI/Overlay/overlay_input.cpp','Extension/Multiplayer/Session/session_internal.h','Extension/Multiplayer/Hud/native_party_internal.h','Extension/Multiplayer/Remote/native_skater_internal.h','External/imgui/imgui.h','External/imgui/imgui_internal.h','External/imgui/imconfig.h']
sources.insert(0,'Engine/Game/Input/playstation_report.h')
sources.insert(1,'Engine/Game/Multiplayer/session_model.h')
hashes={f:hashlib.sha256((a.upstream/f).read_bytes()).hexdigest() for f in sources}
assert (ROOT/'vendor'/sources[0]).read_bytes()==(a.upstream/sources[0]).read_bytes(),'2.1.4-5 report helper'
# Sharing the ImGui context requires identical compiled declarations/config.
for f in sources[-3:]:assert (ROOT/'vendor'/f).read_bytes()==(a.upstream/f).read_bytes(),f
for name in ('engine.h','runtime.h','camera_observer.h','client_source_spawn.h','native_skater.h'):
 f=ROOT/'vendor/Engine/Game/Build/20260929'/name
 def clean(text):return re.sub(r'\s+','',re.sub(r'/\*.*?\*/|//[^\n]*','',text,flags=re.S))
 upstream=clean((a.upstream/'Engine/Game/Build/20260929'/f.name).read_text(encoding='utf-8-sig'))
 for declaration in re.findall(r'inline\s+constexpr\s+[^;]+;',f.read_text(encoding='utf-8-sig')):
  assert clean(declaration) in upstream,f.name+' existing game contract: '+declaration
# Validate ABI declarations independently of address/prefix matching.
def declaration(path,kind,name):
 text=re.sub(r'/\*.*?\*/|//[^\n]*','',path.read_text(encoding='utf-8-sig'),flags=re.S)
 match=re.search(r'\b'+kind+r'\s+'+name+r'\s*\{',text);assert match,(path,name)
 start=match.end()-1;depth=1;end=start+1
 while depth:depth+=(text[end]=='{')-(text[end]=='}');end+=1
 return re.sub(r'\s+','',text[start:end])
for record in json.loads((baseline/'declaration-hashes.json').read_text()):
 actual=declaration(a.upstream/record['file'],record['kind'],record['name'])
 assert hashlib.sha256(actual.encode()).hexdigest()==record['sha256'],('ABI',record['name'])
# All sampled MSC fields must agree with the independently compiled layout.
for expression,key in [('session+787688','Session.mutex'),('session+784664','Session.mode'),('session+784720','Session.host_id'),('session+784704','Session.epoch'),('session+784696','Session.secret'),('session+783664','Session.used_slots')]:
 assert expression in msc and str(int(expression.split('+')[1]))==after[key],key
assert 'i*3136' in msc and after['Peer.size']=='3136'
for offset,key in [(648,'MultiplayerModel.lobby_name'),(380,'MultiplayerModel.dedicated'),(882,'MultiplayerModel.connected')]:
 assert f'session+785352+{offset}' in msc and after[key]==str(offset),key
assert '? 0x5fd8:' in (ROOT/'src/stock_contract.h').read_text() and after['M.bindings']=='24536','bindings offset'
report={'revision':'8320081','dll_sha256':im.digest,'function_prefixes':len(entries),'layout_fields':len(after),'changed_layout_fields':{},'changed_function_addresses':0,'changed_global_addresses_or_call_sites':0,'code_section_sha256':hashlib.sha256(code_section.get_data()).hexdigest(),'source_sha256':hashes,'relationships':'PASS'}
(folder/'audit-report.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
