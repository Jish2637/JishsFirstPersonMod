"""Resolve callbacks with changed local stack frames; preserve non-stack operands."""
import sys,pathlib,json,re,struct
ROOT=pathlib.Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT.parent/'.trace-deps'))
from inspect_hooks import *
from capstone.x86 import X86_REG_RSP,X86_REG_RBP
old=Image(ROOT.parent/'upstream/ReSkate-2.1.4.dll');new=Image(ROOT.parent/'upstream/ReSkate-2.1.4-2.dll')
out=ROOT/'tools/migration214_2';base=json.loads((ROOT/'tools/migration214/contracts.json').read_text());entries=json.loads((out/'contracts.json').read_text());slots=json.loads((out/'data-contracts.json').read_text())
def add(role,rva):
 e=base[role];entries[role]={'old':e['rva'],'rva':rva,'prefix':new.pe.get_data(rva,len(bytes.fromhex(e['prefix']))).hex()};print(role,hex(rva),flush=True)
at=base['msc_session']['rva'];changes={}
for i in old.code(at,150):
 if i.mnemonic=='mov' and i.op_str.startswith('ecx, 0xbf530'):changes[i.address-at+i.imm_offset]=(783664,783704)
add('msc_session',signature(old,new,at,150,changes))
slots['session']=destination(next(i for i in new.code(entries['msc_session']['rva'],100) if i.op_str.startswith('rax, qword ptr [rip')))
for role in ('read','msc_command'):
 at=base[role]['rva'];code=old.code(at,96);size=code[-1].address+code[-1].size-at;data=old.pe.get_data(at,size);wild=set()
 for i in code:
  off=i.address-at
  if rip(i) or (i.mnemonic=='lea' and i.op_str.startswith('rbp,')) or any(o.type==X86_OP_MEM and o.mem.base in (X86_REG_RSP,X86_REG_RBP) for o in i.operands):wild.update(range(off+i.disp_offset,off+i.disp_offset+i.disp_size))
  if i.mnemonic in ('call','jmp') or i.mnemonic.startswith('j') or (i.mnemonic=='mov' and i.op_str.startswith('eax,') and off<20) or (i.mnemonic=='sub' and i.op_str.startswith('rsp,')):wild.update(range(off+i.imm_offset,off+i.imm_offset+i.imm_size))
 pattern=b''.join(b'.' if k in wild else re.escape(bytes([v])) for k,v in enumerate(data));found=[]
 for sec in new.pe.sections:
  if sec.Characteristics&0x20000000:found.extend(sec.VirtualAddress+m.start() for m in re.finditer(pattern,sec.get_data(),re.DOTALL))
 add(role,unique(found,role+' instruction signature with stack operands masked'))
assert len(entries)==33,len(entries)
(out/'contracts.json').write_bytes((json.dumps(entries,indent=2)+'\n').encode());(out/'data-contracts.json').write_bytes((json.dumps(slots,indent=2)+'\n').encode())
for role in ('read','msc_command','msc_session'):
 print('DISASM',role,flush=True)
 for i in new.code(entries[role]['rva'],220):print(hex(i.address),i.mnemonic,i.op_str,flush=True)
