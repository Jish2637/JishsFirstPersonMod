"""Offline 2.1.4-2 candidate discovery, with no DLL loading or guessing shifts."""
import sys,pathlib,json
ROOT=pathlib.Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT.parent/'.trace-deps'))
from inspect_hooks import Image,signature,calls,source_initializer
old=Image(ROOT.parent/'upstream/ReSkate-2.1.4.dll');new=Image(ROOT.parent/'upstream/ReSkate-2.1.4-2.dll')
out=ROOT/'tools/migration214_2';out.mkdir(exist_ok=True)
baseline=json.loads((ROOT/'tools/migration214/contracts.json').read_text());result={}
def add(role,rva):
 e=baseline[role];result[role]={'old':e['rva'],'rva':rva,'prefix':new.pe.get_data(rva,len(bytes.fromhex(e['prefix']))).hex()};print(role,hex(e['rva']),'->',hex(rva),flush=True)
for role,e in baseline.items():
 if role in ('context','getter','source_init'):continue
 try:add(role,signature(old,new,e['rva'],150 if role in ('party_getter','msc_session') else 96))
 except Exception as ex:print('UNRESOLVED',role,ex,flush=True)
add('getter',calls(new.code(new.export('DingoSDKOverlayGetStatus'),180))[0]);add('source_init',source_initializer(new))
(out/'contracts.json').write_bytes((json.dumps(result,indent=2)+'\n').encode())
for role in ('getter','source_init'):
 print('DISASM',role,flush=True)
 for i in new.code(result[role]['rva'],170):print(hex(i.address-result[role]['rva']),i.mnemonic,i.op_str,flush=True)
print('STATUS',hex(new.export('DingoSDKOverlayGetStatus')),flush=True)
