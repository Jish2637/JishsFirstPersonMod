import sys,pathlib,json
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT.parent/'.trace-deps'))
from inspect_hooks import *
old=Image(ROOT.parent/'upstream/ReSkate-2.1.2b.dll');new=Image(ROOT.parent/'upstream/ReSkate-2.1.3.dll')
out=ROOT/'tools/migration213'
baseline=json.loads((ROOT/'tools/migration212b/contracts.json').read_text())
result=json.loads((out/'contracts.json').read_text())
def add(name,rva):
 e=baseline[name];result[name]={'old':e['rva'],'rva':rva,'prefix':new.pe.get_data(rva,len(bytes.fromhex(e['prefix']))).hex()}
 print(name,hex(rva),flush=True)
def dump(label,image,at,size=150):
 print(label,hex(at),flush=True)
 for i in image.code(at,size): print(hex(i.address),i.mnemonic,i.op_str,flush=True)
add('getter',calls(new.code(result['status']['rva'],180))[0])
add('source_init',source_initializer(new))
for name in ('remote_getter','party_getter','msc_session','render','gamepad','hid'):
 for size in (64,48,32,24,16):
  try:
   add(name,signature(old,new,baseline[name]['rva'],size));break
  except Exception as ex:
   if size==16: print(name,ex,flush=True)
add('context',0x1279dc0)
(out/'contracts.json').write_text(json.dumps(result,indent=2)+'\n')
for name in ('getter','source_init','remote_getter','party_getter','msc_session','render','gamepad','hid','mem_alloc','mem_free','context'):
 if name in result: dump(name,new,result[name]['rva'],160)
