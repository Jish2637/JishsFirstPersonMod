import sys,pathlib,json,re,struct
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT.parent/'.trace-deps'))
from inspect_hooks import *
old=Image(ROOT.parent/'upstream/ReSkate-2.1.2b.dll');new=Image(ROOT.parent/'upstream/ReSkate-2.1.3.dll')
out=ROOT/'tools/migration213';baseline=json.loads((ROOT/'tools/migration212b/contracts.json').read_text());result=json.loads((out/'contracts.json').read_text())
def add(name,rva):
 e=baseline[name];result[name]={'old':e['rva'],'rva':rva,'prefix':new.pe.get_data(rva,len(bytes.fromhex(e['prefix']))).hex()};print(name,hex(rva),flush=True)
for name in ('remote_getter','party_getter','msc_session'):
 at=baseline[name]['rva'];changes={}
 for i in old.code(at,150):
  if i.imm_size==4 and any(o.type==X86_OP_IMM and o.imm==0x10160 for o in i.operands):changes[i.address-at+i.imm_offset]=(0x10160,0x10180)
 add(name,signature(old,new,at,150,changes))
for size in (31,26,24,23):
 try: add('hid',signature(old,new,baseline['hid']['rva'],size));break
 except Exception as e: print('hid',size,str(e)[:180],flush=True)
slots={}
for name,role in [('state','getter'),('party','party_getter'),('remote','remote_getter'),('session','msc_session')]:
 ins=next(i for i in new.code(result[role]['rva'],96) if i.op_str.startswith('rax, qword ptr [rip'))
 slots[name]=destination(ins)
refs=[destination(i) for i in new.code(result['mem_alloc']['rva'],50) if rip(i)]
slots['alloc'],slots['userdata'],slots['context']=refs
slots['free']=slots['alloc']+8
code=new.code(result['source_init']['rva'],180)
slots['source_initialized']=destination(next(i for i in code if i.mnemonic=='mov' and rip(i) and i.op_str.endswith(', 1')))
slots['source_base']=destination(next(i for i in code if i.mnemonic=='mov' and rip(i) and i.op_str.endswith(', rdi')))
slots['source_transform']=destination(next(i for i in code if i.address>result['source_init']['rva']+0x7a and i.mnemonic=='mov' and rip(i) and i.op_str.endswith(', rax')))
oldslots=json.loads((ROOT/'tools/migration212b/data-contracts.json').read_text())
for key in ('party_transform_return','party_fov_return'):
 start=oldslots['party_transform_return']-17
 at=signature(old,new,start,47)
 slots[key]=at+oldslots[key]-start
slots['hid_callers']=[]
for at in oldslots['hid_callers']:
 try:
  mapped=signature(old,new,at-16,48)+16;slots['hid_callers'].append(mapped)
 except Exception as e:print('caller failed',hex(at),str(e)[:180],flush=True)
for k,v in slots.items():print(k,hex(v) if isinstance(v,int) else v,flush=True)
(out/'contracts.json').write_text(json.dumps(result,indent=2)+'\n');(out/'data-contracts.json').write_text(json.dumps(slots,indent=2)+'\n')
for name in ('hid','party_getter','remote_getter','msc_session'):
 if name not in result:continue
 print('DISASM',name,flush=True)
 for i in new.code(result[name]['rva'],120):print(hex(i.address),i.mnemonic,i.op_str,flush=True)
