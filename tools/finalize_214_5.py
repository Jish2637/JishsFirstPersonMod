import sys,pathlib,json,struct,re
ROOT=pathlib.Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT.parent/'.trace-deps'))
from inspect_hooks import *
old=Image(ROOT.parent/'upstream/ReSkate-2.1.4-4.dll');new=Image(ROOT.parent/'upstream/ReSkate-2.1.4-5.dll');out=ROOT/'tools/migration214_5';base=json.loads((ROOT/'tools/migration214_4/contracts.json').read_text());e=json.loads((out/'contracts.json').read_text())
slots={}
for key,role in [('state','getter'),('party','party_getter'),('remote','remote_getter'),('session','msc_session')]:
 code=new.code(e[role]['rva'],130)
 if key=='remote':slots[key]=destination(next(i for i in code if i.mnemonic=='add' and rip(i)))
 else:slots[key]=destination(next(i for i in code if i.op_str.startswith('rax, qword ptr [rip')))
slots['alloc'],slots['userdata'],slots['context']=[destination(i) for i in new.code(e['mem_alloc']['rva'],50) if rip(i)];slots['free']=slots['alloc']+8
found=[]
for sec in new.pe.sections:
 if not sec.Characteristics&0x20000000:continue
 data=sec.get_data()
 for m in re.finditer(b'\x48\x8b\x05....\xc3',data,re.DOTALL):
  at=sec.VirtualAddress+m.start()
  if at+7+struct.unpack_from('<i',data,m.start()+3)[0]==slots['context']:found.append(at)
at=unique(found,'ImGui context');e['context']={'old':base['context']['rva'],'rva':at,'prefix':new.pe.get_data(at,8).hex()}
code=new.code(e['source_init']['rva'],180)
slots['source_initialized']=destination(next(i for i in code if i.mnemonic=='mov' and rip(i) and i.op_str.endswith(', 1')))
slots['source_base']=destination(next(i for i in code if i.mnemonic=='mov' and rip(i) and i.op_str.endswith(', rdi')))
slots['source_transform']=destination(next(i for i in code if i.address>e['source_init']['rva']+0x7a and i.mnemonic=='mov' and rip(i) and i.op_str.endswith(', rax')))
prev=json.loads((ROOT/'tools/migration214_4/data-contracts.json').read_text());start=prev['party_transform_return']-17;at=signature(old,new,start,47)
for key in ('party_transform_return','party_fov_return'):slots[key]=at+prev[key]-start
slots['hid_callers']=[]
for sec in new.pe.sections:
 if not sec.Characteristics&0x20000000:continue
 data=sec.get_data()
 for m in re.finditer(b'\xe8',data):
  if m.start()+5<=len(data) and sec.VirtualAddress+m.start()+5+struct.unpack_from('<i',data,m.start()+1)[0]==e['hid']['rva']:slots['hid_callers'].append(sec.VirtualAddress+m.start())
assert len(e)==33;assert len(slots['hid_callers'])==3
for name,value in [('contracts',e),('data-contracts',slots)]: (out/(name+'.json')).write_bytes((json.dumps(value,indent=2)+'\n').encode())
print(json.dumps(slots,indent=2))
