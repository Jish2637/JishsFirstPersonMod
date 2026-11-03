import sys,pathlib,json,re,struct
ROOT=pathlib.Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT.parent/'.trace-deps'))
from inspect_hooks import *
old=Image(ROOT.parent/'upstream/ReSkate-2.1.3.dll');new=Image(ROOT.parent/'upstream/ReSkate-2.1.4.dll')
out=ROOT/'tools/migration214';entries=json.loads((out/'contracts.json').read_text());slots=json.loads((out/'data-contracts.json').read_text());baseline=json.loads((ROOT/'tools/migration213/contracts.json').read_text())
def add(role,rva):
 e=baseline[role];entries[role]={'old':e['rva'],'rva':rva,'prefix':new.pe.get_data(rva,len(bytes.fromhex(e['prefix']))).hex()};print(role,hex(rva),flush=True)
at=baseline['remote_getter']['rva'];changes={}
for i in old.code(at,100):
 if i.imm_size==4:
  for op in i.operands:
   if op.type==X86_OP_IMM and op.imm in (0x10180,888,0x10):changes[i.address-at+i.imm_offset]=(op.imm,{0x10180:0x101b0,888:896,0x10:0x20}[op.imm])
add('remote_getter',signature(old,new,at,96,changes))
slots['remote']=destination(next(i for i in new.code(entries['remote_getter']['rva'],100) if i.op_str.startswith('rax, qword ptr [rip')))
needle=b'Menu preview stick hold reached a native PlayStation HID read.';strings=[];found=[]
for sec in new.pe.sections:
 pos=sec.get_data().find(needle)
 if pos>=0:strings.append(sec.VirtualAddress+pos)
assert len(strings)==1,strings
for sec in new.pe.sections:
 if not sec.Characteristics&0x20000000:continue
 data=sec.get_data()
 for m in re.finditer(b'\x48\x8d[\x05\x0d\x15\x1d\x25\x2d\x35\x3d]',data):
  at=sec.VirtualAddress+m.start()
  if at+7+struct.unpack_from('<i',data,m.start()+3)[0]==strings[0]:found.append(new.function(at)[0])
add('hid',unique(found,'shared HID handler by diagnostic reference'))
slots['hid_callers']=[]
for sec in new.pe.sections:
 if not sec.Characteristics&0x20000000:continue
 data=sec.get_data()
 for m in re.finditer(b'\xe8',data):
  if m.start()+5<=len(data) and sec.VirtualAddress+m.start()+5+struct.unpack_from('<i',data,m.start()+1)[0]==entries['hid']['rva']:slots['hid_callers'].append(sec.VirtualAddress+m.start())
assert len(slots['hid_callers'])==3,slots['hid_callers']
assert len(entries)==32,len(entries)
(out/'contracts.json').write_bytes((json.dumps(entries,indent=2)+'\n').encode());(out/'data-contracts.json').write_bytes((json.dumps(slots,indent=2)+'\n').encode())
for role in ('hid','remote_getter'):
 print('DISASM',role,flush=True)
 for i in new.code(entries[role]['rva'],500 if role=='hid' else 160):print(hex(i.address),i.mnemonic,i.op_str,flush=True)
for call in slots['hid_callers']:print('HID_CALLER',hex(call),tuple(hex(v) for v in new.function(call)),flush=True)
