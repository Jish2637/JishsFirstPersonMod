"""Read-only, independent binary-contract audit. Never loads the reference DLL.
Usage: python tools/audit_210.py --dll ... --upstream ... [--baseline ...]
Requires pefile and capstone (pip install pefile capstone).
"""
import argparse,hashlib,json,re,struct
from pathlib import Path
from inspect_hooks import Image,calls,destination

def run():
 p=argparse.ArgumentParser();p.add_argument('--dll',required=True,type=Path);p.add_argument('--upstream',required=True,type=Path);p.add_argument('--baseline',type=Path);a=p.parse_args()
 im=Image(a.dll);assert im.digest=='310def046836772ef0dea92cfede10e524f4b7fe30255b470e85f2dcbab8c520','unreviewed DLL'
 manifest=json.loads((Path(__file__).parent/'migration210/contracts.json').read_text())
 for name,v in manifest.items():
  assert im.section(v['rva'],32,0x20000000),name
  assert im.pe.get_data(v['rva'],32).hex()==v['prefix'],name
 assert im.export('DingoSDKOverlayGetStatus')==0xde5130
 assert calls(im.code(0xde5130,170))==[0xde49e0,0x11570f4,0x1157120],'status/getter/mutex ABI'
 def relative(at,disp,size):return at+size+struct.unpack('<i',im.pe.get_data(at+disp,4))[0]
 assert relative(0xde4a07,3,7)==0x1862298,'state pointer'
 ctor=im.code(0xde1880,1800)
 assert any(i.mnemonic=='mov' and i.op_str=='qword ptr [rdi + 0x5b0], rsi' for i in ctor),'callback layout'
 assert any(i.mnemonic=='lea' and i.op_str=='rcx, [rdi + 0x5e0]' for i in ctor),'model layout'
 init=im.code(0xbd8a10,170)
 refs={destination(i) for i in init if '[rip ' in i.op_str}
 assert {0x17652c0,0x17661f8,0x1766208}.issubset(refs),'camera initializer slots'
 assert im.pe.get_data(0xbdba2d,2)==b'\xff\xd0','party transform call ABI'
 assert relative(0xbdba4a,1,5)==0xbe1800,'party FOV writer'
 for caller in [0xdf348e,0xdf3d7b]:assert relative(caller,1,5)==0xdf59e0,'HID call contract'
 assert any(i.mnemonic=='call' and i.op_str=='0xde8e30' for i in im.code(0xde92b0,200)),'stock gamepad sampler'
 # Source pins and unchanged maths are separate from runtime behavioural tests.
 source=a.upstream
 expected=['Engine/Game/Skater/first_person_spring.h','Extension/Skater/client_first_person.cpp','Engine/Game/Build/image_identity.cpp','Engine/Game/Build/20260929/engine.h']
 hashes={f:hashlib.sha256((source/f).read_bytes()).hexdigest() for f in expected}
 if a.baseline:
  for f in expected[:2]:assert (source/f).read_bytes()==(a.baseline/f).read_bytes(),f+' camera maths changed'
 protocol=(source/'Extension/Multiplayer/Protocol/protocol.h')
 # Protocol value is logged from the authoritative source, never patched here.
 matches=[]
 for f in (source/'Extension/Multiplayer').rglob('*.h'):
  for line in f.read_text(encoding='utf-8-sig').splitlines():
   if re.search(r'\bprotocol_version\s*=\s*52\b',line):matches.append(str(f.relative_to(source))+': '+line.strip())
 assert matches,'protocol 52 declaration not found'
 print(json.dumps({'dll_sha256':im.digest,'native_entry_prefixes':len(manifest),'relationships':'PASS','source_hashes':hashes,'protocol':matches},indent=2))
if __name__=='__main__':run()
