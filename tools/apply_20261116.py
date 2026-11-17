"""Apply the reviewed 20261116 migration to the isolated source tree once."""
import sys,pathlib,json,re
ROOT=pathlib.Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT.parent/'.trace-deps'))
from inspect_hooks import Image
im=Image(ROOT.parent/'upstream/ReSkate-20261116.dll');out=ROOT/'tools/migration20261116'
entries=json.loads((out/'contracts.json').read_text());slots=json.loads((out/'data-contracts.json').read_text());oldslots=json.loads((ROOT/'tools/migration214_5/data-contracts.json').read_text())
assert len(entries)==33 and len(slots['hid_callers'])==3
mapping={e['old']:e['rva'] for e in entries.values()};mapping.update({oldslots[k]:v for k,v in slots.items() if isinstance(v,int)});mapping[0x9da4e7]=entries['msc_session']['rva']+0x27
paths=list((ROOT/'src').glob('*'))+list((ROOT/'tests').glob('*.cpp'))+[ROOT/'CMakeLists.txt',ROOT/'package.ps1',ROOT/'Mods/JishsFirstPersonMod/manifest.json',ROOT/'tools/release_package_audit.cpp']
for p in paths:
 if not p.is_file() or p.suffix not in ('.cpp','.h','.inl','.txt','.ps1','.json'):continue
 text=p.read_text(encoding='utf-8-sig').replace('1.36.11','1.36.12').replace('2.1.4-5','20261116').replace('214_5','20261116')
 text=text.replace('0820fa71c1cb716f4222eb9027130933aa9e3db4ce20b46ef555760be997dbde',im.digest)
 text=re.sub(r'\b0x[0-9a-fA-F]+\b',lambda m:hex(mapping[int(m[0],16)]) if int(m[0],16) in mapping else m[0],text)
 if p.name in ('native_211.h','embedded_contract.h'):
  text=re.sub(r'(Entry\s+\w+\{"([^"]+)",\s*0x[0-9a-f]+,\s*")[0-9a-f]+("})',lambda m:m[1]+entries[m[2]]['prefix']+m[3],text)
 if p.name=='native_211.h':text=text.replace('debug_revision_offset=26224','debug_revision_offset=26288')
 if p.name=='embedded_contract.h':text=text.replace('state_menu=29064','state_menu=29128')
 if p.name=='spectator_contract.h':
  for label,role in {'party':'party_getter','remote':'remote_getter','render':'render','transform':None}.items():
   at=entries[role]['rva'] if role else slots['party_transform_return']-17
   text=re.sub(r'('+label+r'_signature=")([0-9a-f]+)(")',lambda m:m[1]+im.pe.get_data(at,len(m[2])//2).hex()+m[3],text)
 if p.name=='msc_presence_bridge.cpp':
  def guard(m):return m[1]+','.join(hex(v) for v in im.pe.get_data(int(m[2],16),int(m[3])))+m[4]
  text=re.sub(r'(bytes\(base\+(0x[0-9a-f]+),std::array<unsigned char,(\d+)>\{)[^}]+(})',guard,text)
 if p.name=='stock_hid_contract.h':
  text=re.sub(r'(stock_hid_prefix\{)[^}]+',lambda m:m[1]+','.join(hex(v) for v in im.pe.get_data(entries['hid']['rva'],54)),text)
  text=re.sub(r'stock_hid_callers\{[^}]+}', 'stock_hid_callers{'+','.join(hex(v) for v in slots['hid_callers'])+'}',text)
 if p.name in ('compat214_5_tests.cpp','compat211_tests.cpp'):
  text=re.sub(r'\b(26248|26224|26232)\b',lambda m:str(int(m[0])+64),text)
 if p.name in ('stock_contract.h','stock_contract_tests.cpp'):text=text.replace('0x5fd8','0x6018')
 if text!=p.read_text(encoding='utf-8-sig'):p.write_bytes(text.encode())
p=ROOT/'tests/compat214_5_tests.cpp'
s=p.read_text().replace('2.1.4-4','2.1.4-5').replace('1bcd400b6c487f497ab0110263444864c0269a6309f7143d22b5bc04863d544e','0820fa71c1cb716f4222eb9027130933aa9e3db4ce20b46ef555760be997dbde')
p.write_bytes(s.encode())
p.rename(ROOT/'tests/compat20261116_tests.cpp')
p=ROOT/'CMakeLists.txt';s=p.read_text().replace('upstream/ReSkate-2.1.4-4.dll','upstream/ReSkate-2.1.4-5.dll');p.write_bytes(s.encode())
