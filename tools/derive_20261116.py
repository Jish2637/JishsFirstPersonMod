"""Offline 20261116 candidate discovery, with no DLL loading or guessing shifts."""
import sys,pathlib,json
ROOT=pathlib.Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT.parent/'.trace-deps'))
from inspect_hooks import Image,signature,calls,source_initializer
old=Image(ROOT.parent/'upstream/ReSkate-2.1.4-5.dll');new=Image(ROOT.parent/'upstream/ReSkate-20261116.dll')
out=ROOT/'tools/migration20261116';out.mkdir(exist_ok=True)
baseline=json.loads((ROOT/'tools/migration214_5/contracts.json').read_text());result={}
def add(role,rva):
 e=baseline[role];result[role]={'old':e['rva'],'rva':rva,'prefix':new.pe.get_data(rva,len(bytes.fromhex(e['prefix']))).hex()};print(role,hex(e['rva']),'->',hex(rva),flush=True)
for role,e in baseline.items():
 if role in ('context','getter','source_init'):continue
 try:
  at=e['rva']
  if role in ('party_getter','msc_session','remote_getter'):
   getter=calls(new.code(new.export('DingoSDKOverlayGetStatus'),180))[0]
   getter_code=new.code(getter,0x27)
   tls=next(i.operands[1].imm for i in getter_code if i.mnemonic=='mov' and i.op_str.startswith('edx, 0x'))
   patches={i.address-at+i.imm_offset:(0x10240,tls) for i in old.code(at,120)
            if i.mnemonic=='mov' and i.imm_size==4 and i.op_str.endswith(', 0x10240')}
   add(role,signature(old,new,at,150 if role!='remote_getter' else 96,patches))
  else:add(role,signature(old,new,at,72 if role=='gamepad' else 96))
 except Exception as ex:print('UNRESOLVED',role,ex,flush=True)
add('getter',calls(new.code(new.export('DingoSDKOverlayGetStatus'),180))[0]);add('source_init',source_initializer(new))
(out/'contracts.json').write_bytes((json.dumps(result,indent=2)+'\n').encode())
for role in ('getter','source_init'):
 print('DISASM',role,flush=True)
 for i in new.code(result[role]['rva'],170):print(hex(i.address-result[role]['rva']),i.mnemonic,i.op_str,flush=True)
print('STATUS',hex(new.export('DingoSDKOverlayGetStatus')),flush=True)
