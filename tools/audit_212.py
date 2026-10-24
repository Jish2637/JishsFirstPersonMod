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
assert i.digest == '60be9605476fdd76f3cc2e4b130dda57d645bf3e9d0246c73137884b70cd7fce', i.digest
contracts = json.loads((ROOT/'tools/migration212/contracts.json').read_text())
for name, entry in contracts.items():
    expected = bytes.fromhex(entry['prefix'])
    assert i.pe.get_data(entry['rva'], len(expected)) == expected, name
assert i.export('DingoSDKOverlayGetStatus') == contracts['status']['rva']
for header in ('native_211.h', 'embedded_contract.h'):
    source = (ROOT/'src'/header).read_text()
    for role, rva, prefix in re.findall(r'Entry\s+\w+\{"([^"]+)",\s*(0x[0-9a-f]+),\s*"([0-9a-f]+)"', source):
        expected = bytes.fromhex(prefix)
        assert i.pe.get_data(int(rva,16), len(expected)) == expected, (header, role)
slots = json.loads((ROOT/'tools/migration212/data-contracts.json').read_text())
for site, slot in ((0xf12917,'state'), (0x9d4b17,'session'), (0xb00c07,'party')):
    ins = i.code(site, 7)[0]
    assert ins.address + ins.size + ins.operands[-1].mem.disp == slots[slot], slot
for site in slots['hid_callers']:
    assert i.code(site,5)[0].operands[0].imm == contracts['hid']['rva']
assert i.code(slots['party_fov_return']-5,5)[0].operands[0].imm == contracts['fov']['rva']
print(f'PASS: SHA256, {len(contracts)} function prefixes, native/UI header guards, globals, HID callers and spectator FOV call')
