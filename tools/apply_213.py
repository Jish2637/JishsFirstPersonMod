"""Apply the reviewed 2.1.3 migration to this isolated source tree."""
import sys,pathlib,json,re,struct,hashlib
ROOT=pathlib.Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT.parent/'.trace-deps'))
from inspect_hooks import Image
im=Image(ROOT.parent/'upstream/ReSkate-2.1.3.dll')
out=ROOT/'tools/migration213';entries=json.loads((out/'contracts.json').read_text());slots=json.loads((out/'data-contracts.json').read_text())
entries['hid']={'old':0xf2c110,'rva':0xf98990,'prefix':im.pe.get_data(0xf98990,32).hex()}
slots['hid_callers']=[]
for sec in im.pe.sections:
 if not sec.Characteristics&0x20000000:continue
 data=sec.get_data()
 for m in re.finditer(b'\xe8',data):
  if m.start()+5<=len(data) and sec.VirtualAddress+m.start()+5+struct.unpack_from('<i',data,m.start()+1)[0]==entries['hid']['rva']:
   slots['hid_callers'].append(sec.VirtualAddress+m.start())
assert len(slots['hid_callers'])==3,slots['hid_callers']
oldslots=json.loads((ROOT/'tools/migration212b/data-contracts.json').read_text())
mapping={e['old']:e['rva'] for e in entries.values()}
mapping.update({oldslots[k]:v for k,v in slots.items() if isinstance(v,int)})
mapping[0x9d5e37]=entries['msc_session']['rva']+0x27
paths=list((ROOT/'src').glob('*'))+list((ROOT/'tests').glob('*.cpp'))+[ROOT/'CMakeLists.txt',ROOT/'package.ps1',ROOT/'README.md',ROOT/'LAUNCHER_INSTALL.md',ROOT/'Mods/JishsFirstPersonMod/manifest.json']+list((ROOT/'scripts').rglob('*.lua'))
for p in paths:
 if not p.is_file() or p.suffix not in ('.cpp','.h','.inl','.txt','.ps1','.md','.json','.lua'):continue
 text=p.read_text(encoding='utf-8-sig')
 text=text.replace('1.36.6','1.36.7').replace('2.1.2b.dll','2.1.3.dll').replace('2.1.2','2.1.3').replace('uses_v212_layout','uses_v213_layout')
 text=text.replace('96112d915c181b696b5cb8ed17b62a74d09ca2900c4383a4500c1fa88bf76e43',im.digest)
 text=re.sub(r'\b0x[0-9a-fA-F]+\b',lambda m:hex(mapping.get(int(m[0],16),int(m[0],16))),text)
 text=text.replace('0x6c0','0xac0').replace('0x6f0','0xaf0')
 if p.name in ('native_211.h','embedded_contract.h'):
  text=re.sub(r'(Entry\s+\w+\{"([^"]+)",\s*0x[0-9a-f]+,\s*")[0-9a-f]+("})',lambda m:m[1]+entries[m[2]]['prefix']+m[3],text)
 if p.name=='embedded_contract.h':text=text.replace('state_context=1720,state_menu=27912','state_context=2744,state_menu=28936')
 if p.name=='spectator_contract.h':
  roles={'party':'party_getter','remote':'remote_getter','render':'render','transform':None}
  for label,role in roles.items():
   at=entries[role]['rva'] if role else slots['party_transform_return']-17
   text=re.sub(r'('+label+r'_signature=")([0-9a-f]+)(")',lambda m:m[1]+im.pe.get_data(at,len(m[2])//2).hex()+m[3],text)
 if p.name=='msc_presence_bridge.cpp':
  def guard(m):
   data=im.pe.get_data(int(m[2],16),int(m[3]));return m[1]+','.join(hex(v) for v in data)+m[4]
  text=re.sub(r'(bytes\(base\+(0x[0-9a-f]+),std::array<unsigned char,(\d+)>\{)[^}]+(})',guard,text)
 if p.name=='stock_hid_contract.h':
  text=re.sub(r'(stock_hid_prefix\{)[^}]+',lambda m:m[1]+','.join(hex(v) for v in im.pe.get_data(entries['hid']['rva'],54)),text)
  text=re.sub(r'std::array<std::uintptr_t,2> stock_hid_callers\{[^}]+}', 'std::array<std::uintptr_t,3> stock_hid_callers{'+','.join(hex(v) for v in slots['hid_callers'])+'}',text)
 if p.name=='stick_input_filter.cpp':text=text.replace('size<=512','size<=547')
 p.write_text(text,encoding='utf-8')
(out/'contracts.json').write_text(json.dumps(entries,indent=2)+'\n');(out/'data-contracts.json').write_text(json.dumps(slots,indent=2)+'\n')
print('Applied',len(entries),'functions,',len(mapping),'addresses; HID callers',','.join(hex(v) for v in slots['hid_callers']))
