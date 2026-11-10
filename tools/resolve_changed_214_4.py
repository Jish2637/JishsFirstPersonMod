"""Resolve callbacks with changed local stack frames; preserve non-stack operands."""
import sys,pathlib,json,re,struct
ROOT=pathlib.Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT.parent/'.trace-deps'))
from inspect_hooks import *
from capstone.x86 import X86_REG_RSP,X86_REG_RBP
old=Image(ROOT.parent/'upstream/ReSkate-2.1.4-2.dll');new=Image(ROOT.parent/'upstream/ReSkate-2.1.4-4.dll')
out=ROOT/'tools/migration214_4';base=json.loads((ROOT/'tools/migration214_2/contracts.json').read_text());entries=json.loads((out/'contracts.json').read_text())
def add(role,rva):
 e=base[role];entries[role]={'old':e['rva'],'rva':rva,'prefix':new.pe.get_data(rva,len(bytes.fromhex(e['prefix']))).hex()};print(role,hex(rva),flush=True)

# Only mask reviewed layout constants and compiler stack/TLS operands.
changes={896:912,783704:787768,48704:48704,26120:26216,26136:26232,24520:24536,24704-16:24704,780680:784664,780720:784704,780736:784720,780712:784696,783624:787688,781368:785352,28968:29064}
for role,e in base.items():
 if role in entries or role=='context':continue
 at=e['rva'];code=old.code(at,96);size=code[-1].address+code[-1].size-at;data=bytearray(old.pe.get_data(at,size));wild=set()
 for i in code:
  off=i.address-at
  if rip(i) or (i.mnemonic=='lea' and i.op_str.startswith('rbp,')) or any(o.type==X86_OP_MEM and o.mem.base in (X86_REG_RSP,X86_REG_RBP) for o in i.operands):wild.update(range(off+i.disp_offset,off+i.disp_offset+i.disp_size))
  if i.mnemonic in ('call','jmp') or i.mnemonic.startswith('j') or (i.mnemonic=='mov' and i.op_str.startswith('eax,') and off<20) or (i.mnemonic=='sub' and i.op_str.startswith('rsp,')):wild.update(range(off+i.imm_offset,off+i.imm_offset+i.imm_size))
  for o in i.operands:
   if o.type==X86_OP_MEM and o.mem.disp in changes and i.disp_size==4:struct.pack_into('<i',data,off+i.disp_offset,changes[o.mem.disp])
   if o.type==X86_OP_IMM and o.imm in changes and i.imm_size==4:struct.pack_into('<i',data,off+i.imm_offset,changes[o.imm])
 pattern=b''.join(b'.' if k in wild else re.escape(bytes([v])) for k,v in enumerate(data));found=[]
 for sec in new.pe.sections:
  if sec.Characteristics&0x20000000:found.extend(sec.VirtualAddress+m.start() for m in re.finditer(pattern,sec.get_data(),re.DOTALL))
 try:add(role,unique(found,role+' reviewed normalized signature'))
 except Exception as ex:print('UNRESOLVED',role,ex,flush=True)
(out/'contracts.json').write_bytes((json.dumps(entries,indent=2)+'\n').encode())
for role in base:
 if role in entries:continue
 print('BASE DISASM',role,flush=True)
 for i in old.code(base[role]['rva'],120):print(hex(i.address),i.mnemonic,i.op_str,flush=True)
