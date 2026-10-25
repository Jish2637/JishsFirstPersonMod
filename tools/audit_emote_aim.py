"""Read-only audit of reviewed emote/camera EXE findings; never opens a process."""
import argparse
import hashlib
import json
import pathlib
import struct
import sys
ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT.parent / '.trace-deps'))
from capstone import Cs, CS_ARCH_X86, CS_MODE_64

class Image:
    def __init__(self, path, expected):
        self.data = path.read_bytes()
        if hashlib.sha256(self.data).hexdigest() != expected:
            raise ValueError('Unreviewed game EXE hash')
        nt = struct.unpack_from('<I', self.data, 0x3c)[0]
        if self.data[nt:nt+4] != b'PE\0\0' or struct.unpack_from('<H', self.data, nt+4)[0] != 0x8664:
            raise ValueError('Expected AMD64 PE')
        opt = nt+24
        count = struct.unpack_from('<H', self.data, nt+6)[0]
        size = struct.unpack_from('<H', self.data, nt+20)[0]
        self.base = struct.unpack_from('<Q', self.data, opt+24)[0]
        self.sections = []
        for index in range(count):
            at = opt+size+index*40
            virtual_size, rva, raw_size, raw = struct.unpack_from('<IIII', self.data, at+8)
            self.sections.append((rva, virtual_size, raw_size, raw))
        self.md = Cs(CS_ARCH_X86, CS_MODE_64)
        self.md.detail = True

    def read(self, rva, size):
        for start, virtual_size, raw_size, raw in self.sections:
            delta = rva-start
            if 0 <= delta and size >= 0 and delta+size <= min(virtual_size, raw_size):
                data = self.data[raw+delta:raw+delta+size]
                if len(data) == size:
                    return data
        raise ValueError(f'Not file-backed: {rva:#x}+{size:#x}')

    def pointer(self, rva):
        return struct.unpack('<Q', self.read(rva,8))[0]-self.base

    def text(self, rva):
        return self.read(rva,96).split(b'\0',1)[0].decode('ascii')

    def code(self, rva, size):
        return list(self.md.disasm(self.read(rva,size),rva))

def audit(path):
    manifest = json.loads((ROOT/'tools/emote_aim/findings.json').read_text())
    im = Image(path, manifest['game_sha256'])
    f = manifest['functions']
    def address(name):
        return int(f[name]['rva'],16)
    for name, entry in f.items():
        expected = bytes.fromhex(entry['prefix'])
        assert im.read(int(entry['rva'],16),len(expected)) == expected, name
    assert im.pointer(0x6171ae0) == address('gesture_component_type_getter')
    ins = im.code(address('gesture_component_type_getter'),8)[0]
    assert ins.address+ins.size+ins.disp == 0x72804e0
    for record, function, wrapper, params, counts in (
        (0x7f331e0,'gesturing_player_query','gesture_query_wrapper',0x6121c80,(2,3)),
        (0x7f52d10,'camera_transform_native','camera_transform_wrapper',0x6132d10,(0,1))):
        assert im.pointer(record+0x20) == address(function)
        assert im.pointer(record+0x28) == address(wrapper)
        assert im.pointer(record+0x38) == params
        assert struct.unpack('<HH',im.read(record+0x44,4)) == counts
    assert im.text(im.pointer(0x6121ce0)) == 'PlayingGesture'
    assert im.text(im.pointer(0x6121d00)) == 'GesturingPlayerWorldTransform'
    assert im.text(im.pointer(0x6132d10)) == 'CameraTransform'
    query = im.code(address('gesturing_player_query'),0x132)
    calls = {i.operands[0].imm for i in query if i.mnemonic=='call' and i.operands[0].type==2}
    assert {address('gesture_active_getter'),address('gesture_id_getter')} <= calls
    jump = im.code(address('camera_transform_wrapper'),5)[0]
    assert jump.mnemonic == 'jmp' and jump.operands[0].imm == address('camera_transform_native')
    print('PASS: exact EXE, 10 reviewed prefixes, gesture type/getters, native parameter records and camera wrapper')
    print('Investigation only: local gesture lifecycle and emote-to-camera consumer link remain unverified.')

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe',type=pathlib.Path)
    try:
        audit(parser.parse_args().exe)
    except (ValueError, AssertionError, OSError) as error:
        raise SystemExit(f'FAIL: {error}')
