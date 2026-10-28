import sys,pathlib,json
ROOT=pathlib.Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT.parent/'.trace-deps'))
from inspect_hooks import *
old=Image(ROOT.parent/'upstream/ReSkate-2.1.2b.dll');new=Image(ROOT.parent/'upstream/ReSkate-2.1.3.dll')
for call in (0xf292ae,0xf29bcb):
 start,end=old.function(call);print('OLD CALLER',hex(start),hex(end),flush=True)
 changes={}
 for i in old.code(start,200):
  if i.imm_size==4 and any(o.type==X86_OP_IMM and o.imm in (0x10160,0x280) for o in i.operands):
   value=next(o.imm for o in i.operands if o.type==X86_OP_IMM);changes[i.address-start+i.imm_offset]=(value,value+0x20)
 for size in (180,96,64,48,32):
  try:
   at=signature(old,new,start,size,{k:v for k,v in changes.items() if k+4<=size});print('NEW CALLER',hex(at),flush=True)
   ns,ne=new.function(at)
   for i in new.code(at,ne-at):
    if i.mnemonic=='call':print(hex(i.address),i.mnemonic,i.op_str,flush=True)
   break
  except Exception as ex:print('FAIL',size,str(ex)[:120],flush=True)
# Identify the new shared handler by its unique source diagnostic string.
needle=b'Menu preview stick hold reached a native PlayStation HID read.'
for s in new.pe.sections:
 pos=s.get_data().find(needle)
 if pos>=0:
  target=s.VirtualAddress+pos;print('STRING',hex(target),flush=True)
  for textsec in new.pe.sections:
   if not textsec.Characteristics&0x20000000:continue
   data=textsec.get_data()
   for m in re.finditer(b'\x48\x8d[\x05\x0d\x15\x1d\x25\x2d\x35\x3d]',data):
    at=textsec.VirtualAddress+m.start()
    if at+7+struct.unpack_from('<i',data,m.start()+3)[0]==target:
     print('HANDLER',tuple(hex(x) for x in new.function(at)),flush=True)
     start,end=new.function(at)
     for i in new.code(start,min(end-start,500)):print(hex(i.address),i.mnemonic,i.op_str,flush=True)
