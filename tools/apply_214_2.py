"""Apply the reviewed 2.1.4-2 migration to the isolated source tree once."""
import sys,pathlib,json,re
ROOT=pathlib.Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT.parent/'.trace-deps'))
from inspect_hooks import Image
im=Image(ROOT.parent/'upstream/ReSkate-2.1.4-2.dll');out=ROOT/'tools/migration214_2'
entries=json.loads((out/'contracts.json').read_text());slots=json.loads((out/'data-contracts.json').read_text());oldslots=json.loads((ROOT/'tools/migration214/data-contracts.json').read_text())
assert len(entries)==33 and len(slots['hid_callers'])==3
mapping={e['old']:e['rva'] for e in entries.values()};mapping.update({oldslots[k]:v for k,v in slots.items() if isinstance(v,int)});mapping[0x9e6647]=entries['msc_session']['rva']+0x27
paths=list((ROOT/'src').glob('*'))+list((ROOT/'tests').glob('*.cpp'))+[ROOT/'CMakeLists.txt',ROOT/'package.ps1',ROOT/'Mods/JishsFirstPersonMod/manifest.json',ROOT/'tools/release_package_audit.cpp']
for p in paths:
 if not p.is_file() or p.suffix not in ('.cpp','.h','.inl','.txt','.ps1','.json'):continue
 text=p.read_text(encoding='utf-8-sig').replace('1.36.8','1.36.9').replace('2.1.4','2.1.4-2').replace('uses_v214_layout','uses_v214_2_layout')
 text=text.replace('39195d824d775e5046480d66cb62dae0d709648f145414cb952ed2aa8afb99b3',im.digest)
 text=re.sub(r'\b0x[0-9a-fA-F]+\b',lambda m:hex(mapping[int(m[0],16)]) if int(m[0],16) in mapping else m[0],text)
 if p.name in ('native_211.h','embedded_contract.h'):
  text=re.sub(r'(Entry\s+\w+\{"([^"]+)",\s*0x[0-9a-f]+,\s*")[0-9a-f]+("})',lambda m:m[1]+entries[m[2]]['prefix']+m[3],text)
 if p.name=='native_211.h':text=text.replace('debug_revision_offset=26104','debug_revision_offset=26128')
 if p.name=='embedded_contract.h':text=text.replace('state_menu=28944','state_menu=28968')
 if p.name=='spectator_contract.h':
  for label,role in {'party':'party_getter','remote':'remote_getter','render':'render','transform':None}.items():
   at=entries[role]['rva'] if role else slots['party_transform_return']-17
   text=re.sub(r'('+label+r'_signature=")([0-9a-f]+)(")',lambda m:m[1]+im.pe.get_data(at,len(m[2])//2).hex()+m[3],text)
 if p.name=='msc_presence_bridge.cpp':
  def guard(m):return m[1]+','.join(hex(v) for v in im.pe.get_data(int(m[2],16),int(m[3])))+m[4]
  text=re.sub(r'(bytes\(base\+(0x[0-9a-f]+),std::array<unsigned char,(\d+)>\{)[^}]+(})',guard,text)
  text=text.replace('session+783584','session+783624').replace('session+781352','session+781368').replace('+802)', '+826)')
 if p.name=='stock_hid_contract.h':
  text=re.sub(r'(stock_hid_prefix\{)[^}]+',lambda m:m[1]+','.join(hex(v) for v in im.pe.get_data(entries['hid']['rva'],54)),text)
  text=re.sub(r'stock_hid_callers\{[^}]+}', 'stock_hid_callers{'+','.join(hex(v) for v in slots['hid_callers'])+'}',text)
 if p.name=='compat214_tests.cpp':text=re.sub(r'\b(26128|26104|26112)\b',lambda m:str({26128:26152,26104:26128,26112:26136}[int(m[0])]),text)
 p.write_bytes(text.encode())
p=ROOT/'tests/compat214_tests.cpp';p.rename(ROOT/'tests/compat214_2_tests.cpp')
p=ROOT/'CMakeLists.txt';s=p.read_text().replace('compat214_tests','compat214_2_tests').replace('official_214_','official_214_2_').replace('upstream/ReSkate-2.1.3.dll','upstream/ReSkate-2.1.4.dll');p.write_bytes(s.encode())
p=ROOT/'tests/compat214_2_tests.cpp';s=p.read_text().replace('previous 2.1.3','previous 2.1.4').replace('2.1.3 identity','2.1.4 identity').replace('139e932ad6328b898b2ff8c2aac3e0cd5ced6545c2f041aa8534239f97e59f9b','39195d824d775e5046480d66cb62dae0d709648f145414cb952ed2aa8afb99b3');p.write_bytes(s.encode())
print('Updated 33 entries,',len(mapping),'addresses; model revision, menu, session mutex/view and connected offsets')
