"""Offline audit for the exact reviewed ReSkate 2.1.2 binary. Never loads it."""
import argparse
import json
import pathlib
import re
import sys
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[2] / '.trace-deps'))
from inspect_hooks import Image, calls, destination
ROOT = pathlib.Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser()
p.add_argument('dll', type=pathlib.Path)
a = p.parse_args()
i = Image(a.dll)
assert i.digest == '96112d915c181b696b5cb8ed17b62a74d09ca2900c4383a4500c1fa88bf76e43', i.digest
contracts = json.loads((ROOT/'tools/migration212b/contracts.json').read_text())
for name, entry in contracts.items():
    expected = bytes.fromhex(entry['prefix'])
    assert i.pe.get_data(entry['rva'], len(expected)) == expected, name
assert i.export('DingoSDKOverlayGetStatus') == contracts['status']['rva']
for header in ('native_211.h', 'embedded_contract.h'):
    source = (ROOT/'src'/header).read_text()
    for role, rva, prefix in re.findall(r'Entry\s+\w+\{"([^"]+)",\s*(0x[0-9a-f]+),\s*"([0-9a-f]+)"', source):
        expected = bytes.fromhex(prefix)
        assert i.pe.get_data(int(rva,16), len(expected)) == expected, (header, role)
slots = json.loads((ROOT/'tools/migration212b/data-contracts.json').read_text())
for site, slot in ((0xf1d1b7,'state'), (0x9d5e37,'session'), (0xb02027,'party')):
    ins = i.code(site, 7)[0]
    assert ins.address + ins.size + ins.operands[-1].mem.disp == slots[slot], slot
for site in slots['hid_callers']:
    assert i.code(site,5)[0].operands[0].imm == contracts['hid']['rva']
assert i.code(slots['party_fov_return']-5,5)[0].operands[0].imm == contracts['fov']['rva']
# Check the byte guards actually compiled into the MSC bridge too.
source = (ROOT/'src/msc_presence_bridge.cpp').read_text()
for rva, values in re.findall(r'bytes\(base\+(0x[0-9a-f]+),std::array<unsigned char,\d+>\{([^}]+)}', source):
    expected = bytes(int(v.strip(),0) for v in values.split(','))
    assert i.pe.get_data(int(rva,16),len(expected)) == expected, ('MSC guard',rva)
# The sender independently reads these reviewed Session fields.
code = i.code(contracts['msc_send']['rva'],90)
displacements = {op.mem.disp for ins in code for op in ins.operands if op.type == 3}
assert {780680,780736} <= displacements, displacements
# Ensure the source initializer still publishes the game camera setter.
code = i.code(contracts['source_init']['rva'],180)
assert any('0x2f8fa90' in ins.op_str for ins in code)
for name in ('source_initialized','source_base','source_transform'):
    assert any('[rip' in ins.op_str and destination(ins)==slots[name] for ins in code), name
print(f'PASS: SHA256, {len(contracts)} function prefixes, native/UI header guards, globals, HID callers and spectator FOV call')
