"""Derive candidates and global relationships from reviewed 2.1.4 interfaces."""
import sys,pathlib,json,re,struct
ROOT=pathlib.Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT.parent/'.trace-deps'))
from inspect_hooks import *
old=Image(ROOT.parent/'upstream/ReSkate-2.1.4.dll');new=Image(ROOT.parent/'upstream/ReSkate-2.1.4-2.dll')
out=ROOT/'tools/migration214_2';baseline=json.loads((ROOT/'tools/migration214/contracts.json').read_text());result=json.loads((out/'contracts.json').read_text())
def add(role,rva):
 e=baseline[role];result[role]={'old':e['rva'],'rva':rva,'prefix':new.pe.get_data(rva,len(bytes.fromhex(e['prefix']))).hex()};print(role,hex(rva),flush=True)
for role,allocation in [('party_getter',(48640,48704)),('msc_session',None)]:
 at=baseline[role]['rva'];changes={}
 if allocation:
  for i in old.code(at,150):
   if i.imm_size==4 and any(o.type==X86_OP_IMM and o.imm==allocation[0] for o in i.operands):changes[i.address-at+i.imm_offset]=allocation
 try:add(role,signature(old,new,at,150,changes))
 except Exception as ex:print('UNRESOLVED',role,ex,flush=True)
for role,bytes_count in [('read',25),('msc_command',16)]:
 e=baseline[role];prefix=bytes.fromhex(e['prefix'])[:bytes_count];found=[]
 for sec in new.pe.sections:
  if not sec.Characteristics&0x20000000:continue
  for m in re.finditer(re.escape(prefix),sec.get_data()):found.append(sec.VirtualAddress+m.start())
 try:add(role,unique(found,role+' distinctive prologue'))
 except Exception as ex:print('UNRESOLVED',role,ex,flush=True)
slots={}
for key,role in [('state','getter'),('party','party_getter'),('remote','remote_getter'),('session','msc_session')]:
 if role not in result:continue
 slots[key]=destination(next(i for i in new.code(result[role]['rva'],100) if i.op_str.startswith('rax, qword ptr [rip')))
refs=[destination(i) for i in new.code(result['mem_alloc']['rva'],50) if rip(i)]
slots['alloc'],slots['userdata'],slots['context']=refs;slots['free']=slots['alloc']+8
found=[]
for sec in new.pe.sections:
 if not sec.Characteristics&0x20000000:continue
 data=sec.get_data()
 for m in re.finditer(b'\x48\x8b\x05....\xc3',data,re.DOTALL):
  at=sec.VirtualAddress+m.start()
  if at+7+struct.unpack_from('<i',data,m.start()+3)[0]==slots['context']:found.append(at)
add('context',unique(found,'host ImGui context getter'))
code=new.code(result['source_init']['rva'],180)
slots['source_initialized']=destination(next(i for i in code if i.mnemonic=='mov' and rip(i) and i.op_str.endswith(', 1')))
slots['source_base']=destination(next(i for i in code if i.mnemonic=='mov' and rip(i) and i.op_str.endswith(', rdi')))
slots['source_transform']=destination(next(i for i in code if i.address>result['source_init']['rva']+0x7a and i.mnemonic=='mov' and rip(i) and i.op_str.endswith(', rax')))
oldslots=json.loads((ROOT/'tools/migration214/data-contracts.json').read_text());start=oldslots['party_transform_return']-17;mapped=signature(old,new,start,47)
for key in ('party_transform_return','party_fov_return'):slots[key]=mapped+oldslots[key]-start
slots['hid_callers']=[]
for sec in new.pe.sections:
 if not sec.Characteristics&0x20000000:continue
 data=sec.get_data()
 for m in re.finditer(b'\xe8',data):
  if m.start()+5<=len(data) and sec.VirtualAddress+m.start()+5+struct.unpack_from('<i',data,m.start()+1)[0]==result['hid']['rva']:slots['hid_callers'].append(sec.VirtualAddress+m.start())
(out/'contracts.json').write_bytes((json.dumps(result,indent=2)+'\n').encode());(out/'data-contracts.json').write_bytes((json.dumps(slots,indent=2)+'\n').encode())
for k,v in slots.items():print(k,hex(v) if isinstance(v,int) else [hex(x) for x in v],flush=True)
for role in ('read','msc_command','party_getter','msc_session'):
 if role not in result:continue
 print('DISASM',role,flush=True)
 for i in new.code(result[role]['rva'],180):print(hex(i.address-result[role]['rva']),i.mnemonic,i.op_str,flush=True)
