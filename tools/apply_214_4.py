"""Apply the reviewed 2.1.4-2 migration to the isolated source tree once."""
import sys,pathlib,json,re
ROOT=pathlib.Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT.parent/'.trace-deps'))
from inspect_hooks import Image
im=Image(ROOT.parent/'upstream/ReSkate-2.1.4-4.dll');out=ROOT/'tools/migration214_4'
entries=json.loads((out/'contracts.json').read_text());slots=json.loads((out/'data-contracts.json').read_text());oldslots=json.loads((ROOT/'tools/migration214_2/data-contracts.json').read_text())
assert len(entries)==33 and len(slots['hid_callers'])==3
mapping={e['old']:e['rva'] for e in entries.values()};mapping.update({oldslots[k]:v for k,v in slots.items() if isinstance(v,int)});mapping[0x9e6897]=entries['msc_session']['rva']+0x27
paths=list((ROOT/'src').glob('*'))+list((ROOT/'tests').glob('*.cpp'))+[ROOT/'CMakeLists.txt',ROOT/'package.ps1',ROOT/'Mods/JishsFirstPersonMod/manifest.json',ROOT/'tools/release_package_audit.cpp']
for p in paths:
 if not p.is_file() or p.suffix not in ('.cpp','.h','.inl','.txt','.ps1','.json'):continue
 text=p.read_text(encoding='utf-8-sig').replace('1.36.9','1.36.10').replace('2.1.4-2','2.1.4-4').replace('uses_v214_2_layout','uses_v214_4_layout')
 text=text.replace('f7b1562667892187356b8617a3964ceaefea04324bfa7850c05d340da01c09cd',im.digest)
 text=re.sub(r'\b0x[0-9a-fA-F]+\b',lambda m:hex(mapping[int(m[0],16)]) if int(m[0],16) in mapping else m[0],text)
 if p.name in ('native_211.h','embedded_contract.h'):
  text=re.sub(r'(Entry\s+\w+\{"([^"]+)",\s*0x[0-9a-f]+,\s*")[0-9a-f]+("})',lambda m:m[1]+entries[m[2]]['prefix']+m[3],text)
 if p.name=='native_211.h':text=text.replace('debug_revision_offset=26128','debug_revision_offset=26224')
 if p.name=='embedded_contract.h':text=text.replace('state_menu=28968','state_menu=29064')
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
 if p.name in ('compat214_2_tests.cpp','compat211_tests.cpp'):
  text=re.sub(r'\b(26152|26128|26136)\b',lambda m:str({26152:26248,26128:26224,26136:26232}[int(m[0])]),text)
 if p.name=='spectator_runtime_tests.cpp':text=re.sub(r'\b896\b','912',text)
 if p.name=='spectator_contract.h':text=text.replace('remote_stride=896','remote_stride=912')
 if p.name=='stock_contract.h':text=text.replace('? 0x5fc8:', '? 0x5fd8:')
 if p.name=='stock_contract_tests.cpp':text=text.replace('bindings_model_offset()==0x5fc8','bindings_model_offset()==0x5fd8')
 if p.name=='msc_presence_bridge.cpp':
  offsets={783624:787688,780680:784664,780736:784720,780720:784704,780712:784696,781368:785352,779680:783664,592:648,324:380,826:882,3120:3136}
  text=re.sub(r'\b\d+\b',lambda m:str(offsets.get(int(m[0]),int(m[0]))),text)
 p.write_bytes(text.encode())
p=ROOT/'tests/compat214_2_tests.cpp';p.rename(ROOT/'tests/compat214_4_tests.cpp')
p=ROOT/'CMakeLists.txt';s=p.read_text().replace('compat214_2_tests','compat214_4_tests').replace('official_214_2_','official_214_4_').replace('upstream/ReSkate-2.1.4.dll','upstream/ReSkate-2.1.4-2.dll');p.write_bytes(s.encode())
p=ROOT/'tests/compat214_4_tests.cpp';s=p.read_text().replace('previous 2.1.4','previous 2.1.4-2').replace('2.1.4 identity','2.1.4-2 identity').replace('39195d824d775e5046480d66cb62dae0d709648f145414cb952ed2aa8afb99b3','f7b1562667892187356b8617a3964ceaefea04324bfa7850c05d340da01c09cd');p.write_bytes(s.encode())
print('Applied reviewed 2.1.4-4 entries, globals, fields and strides')
