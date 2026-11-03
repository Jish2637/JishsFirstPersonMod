"""Apply reviewed 2.1.4 addresses to this isolated mod checkout."""
import sys,pathlib,json,re,struct
ROOT=pathlib.Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT.parent/'.trace-deps'))
from inspect_hooks import Image,signature
im=Image(ROOT.parent/'upstream/ReSkate-2.1.4.dll')
out=ROOT/'tools/migration214';entries=json.loads((out/'contracts.json').read_text());slots=json.loads((out/'data-contracts.json').read_text())
assert len(entries)==32 and len(slots['hid_callers'])==3
oldslots=json.loads((ROOT/'tools/migration213/data-contracts.json').read_text())
mapping={e['old']:e['rva'] for e in entries.values()};mapping.update({oldslots[k]:v for k,v in slots.items() if isinstance(v,int)})
mapping[0x9d6e57]=entries['msc_session']['rva']+0x27
paths=list((ROOT/'src').glob('*'))+list((ROOT/'tests').glob('*.cpp'))+[ROOT/'CMakeLists.txt',ROOT/'package.ps1',ROOT/'Mods/JishsFirstPersonMod/manifest.json',ROOT/'tools/release_package_audit.cpp']
for p in paths:
 if not p.is_file() or p.suffix not in ('.cpp','.h','.inl','.txt','.ps1','.json'):continue
 text=p.read_text(encoding='utf-8-sig')
 text=text.replace('1.36.7','1.36.8').replace('2.1.3','2.1.4').replace('uses_v213_layout','uses_v214_layout')
 text=text.replace('139e932ad6328b898b2ff8c2aac3e0cd5ced6545c2f041aa8534239f97e59f9b',im.digest)
 text=re.sub(r'\b0x[0-9a-fA-F]+\b',lambda m:hex(mapping[int(m[0],16)]) if int(m[0],16) in mapping else m[0],text)
 if p.name in ('stock_contract.h','stock_contract_tests.cpp'):text=text.replace('0xac0','0xac8').replace('0xaf0','0xaf8')
 if p.name in ('native_211.h','embedded_contract.h'):
  text=re.sub(r'(Entry\s+\w+\{"([^"]+)",\s*0x[0-9a-f]+,\s*")[0-9a-f]+("})',lambda m:m[1]+entries[m[2]]['prefix']+m[3],text)
 if p.name=='embedded_contract.h':text=text.replace('state_context=2744,state_menu=28936,state_window=720','state_context=2752,state_menu=28944,state_window=728')
 if p.name=='spectator_contract.h':
  text=text.replace('remote_stride=888','remote_stride=896')
  for label,role in {'party':'party_getter','remote':'remote_getter','render':'render','transform':None}.items():
   at=entries[role]['rva'] if role else slots['party_transform_return']-17
   text=re.sub(r'('+label+r'_signature=")([0-9a-f]+)(")',lambda m:m[1]+im.pe.get_data(at,len(m[2])//2).hex()+m[3],text)
 if p.name=='msc_presence_bridge.cpp':
  def guard(m):return m[1]+','.join(hex(v) for v in im.pe.get_data(int(m[2],16),int(m[3])))+m[4]
  text=re.sub(r'(bytes\(base\+(0x[0-9a-f]+),std::array<unsigned char,(\d+)>\{)[^}]+(})',guard,text)
 if p.name=='stock_hid_contract.h':
  text=re.sub(r'(stock_hid_prefix\{)[^}]+',lambda m:m[1]+','.join(hex(v) for v in im.pe.get_data(entries['hid']['rva'],54)),text)
  text=re.sub(r'stock_hid_callers\{[^}]+}', 'stock_hid_callers{'+','.join(hex(v) for v in slots['hid_callers'])+'}',text)
 if p.name=='spectator_runtime_tests.cpp':text=text.replace('249*888','249*896').replace('2*888','2*896')
 p.write_bytes(text.encode('utf-8'))
p=ROOT/'tests/compat213_tests.cpp';p.rename(ROOT/'tests/compat214_tests.cpp')
p=ROOT/'CMakeLists.txt';text=p.read_text().replace('compat213_tests','compat214_tests').replace('official_213_','official_214_').replace('upstream/ReSkate-2.1.2b.dll','upstream/ReSkate-2.1.3.dll');p.write_bytes(text.encode())
p=ROOT/'tests/compat214_tests.cpp';text=p.read_text().replace('previous 2.1.2','previous 2.1.3').replace('2.1.2 identity','2.1.3 identity').replace('96112d915c181b696b5cb8ed17b62a74d09ca2900c4383a4500c1fa88bf76e43','139e932ad6328b898b2ff8c2aac3e0cd5ced6545c2f041aa8534239f97e59f9b');p.write_bytes(text.encode())
print('Applied',len(entries),'functions,',len(mapping),'addresses, five overlay offsets and remote stride 896')
