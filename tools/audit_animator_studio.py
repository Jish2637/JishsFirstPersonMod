"""Audit the exact Animator Studio 1.0.101 asset without loading it.
Usage: python tools/audit_animator_studio.py DLL --upstream SOURCE
Requires pefile and capstone (tools/requirements.txt).
"""
import argparse,hashlib,json,pathlib,re,struct,sys
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT.parent/'.trace-deps'))
from inspect_hooks import Image,calls,destination,rip
p=argparse.ArgumentParser();p.add_argument('dll',type=pathlib.Path);p.add_argument('--upstream',type=pathlib.Path,required=True);a=p.parse_args()
im=Image(a.dll);assert im.digest=='ed8f07863bcc3de1d83371de0f90b368ca55602d793d1bd84dbbff44b6d76801','unreviewed DLL'
folder=ROOT/'tools/animator_studio';entries=json.loads((folder/'contracts.json').read_text());slots=json.loads((folder/'data-contracts.json').read_text())
def address(n):return entries[n]['rva']
def relative(at,disp=1,size=5):return at+size+struct.unpack('<i',im.pe.get_data(at+disp,4))[0]
for name,e in entries.items():
 b=bytes.fromhex(e['prefix']);assert im.section(e['rva'],len(b),0x20000000),name
 assert im.pe.get_data(e['rva'],len(b))==b,name
assert im.export('DingoSDKOverlayGetStatus')==address('status')
assert calls(im.code(address('status'),180))==[address('getter'),address('lock'),address('unlock')]
assert relative(address('getter')+0x27,3,7)==slots['state']
for offset,slot in [(0x5a,'source_initialized'),(0x7b,'source_transform'),(0x82,'source_base')]:
 assert relative(address('source_init')+offset,3,7)==slots[slot]
for name,value in slots.items():
 if name in ('party_transform_return','party_fov_return','hid_callers'):continue
 assert im.section(value,8,0xc0000000),name
assert relative(slots['party_fov_return']-5)==address('fov')
assert im.pe.get_data(slots['party_transform_return']-2,2)==b'\xff\xd0'
assert len(slots['hid_callers'])==3
for at in slots['hid_callers']:assert relative(at)==address('hid')
assert relative(address('controller')+0x1c6)==address('gamepad')
page=im.code(address('draw'),0xd00);pcalls=set(calls(page))
assert {address(n) for n in ('begin_card','end_card','field','note','slider_float','toggle_row')}<=pcalls
assert address('draw') in calls(im.code(address('skater_page'),0x200))
refs={destination(i) for i in page if i.mnemonic=='lea' and rip(i)}
strings={im.pe.get_data(at,96).split(b'\0')[0] for at in refs}
assert {b'first-person',b'FIRST PERSON',b'Field of view',b'Spring arm'}<=strings
# Semantic checks distinguish generic function prologues from camera routines.
fov=im.code(address('fov'),0x54)
assert any(i.op_str=='dword ptr [rcx + 0xac], xmm1' for i in fov)
assert any(i.op_str=='eax, 0x7f800000' for i in fov)
assert any('0x7ffffffefeff' in i.op_str for i in fov)
sample=im.code(address('sampler'),0x200)
assert any(i.op_str=='rbx, rdx' for i in sample) and any(i.op_str=='rsi, rcx' for i in sample)
assert any(i.op_str=='r10, r8' for i in sample) and any(i.op_str=='rdi, r9' for i in sample)
assert any(i.op_str=='eax, 0x7f800000' for i in sample)
assert relative(0xda5b86)==address('sampler'),'first-person caller'
ctor=im.code(address('ctor'),2300)
assert any(i.op_str=='qword ptr [rdi + 0xb68], rsi' for i in ctor)
assert any(i.op_str=='rcx, [rdi + 0xb98]' for i in ctor)
for n in ('mem_alloc','mem_free'):
 refs={destination(i) for i in im.code(address(n),180) if rip(i)}
 assert {slots['context'],slots['userdata'],slots['free' if n=='mem_free' else 'alloc']}<=refs,n
for header in ('native_211.h','embedded_contract.h'):
 for n,at,prefix in re.findall(r'Entry\s+\w+\{"([^"]+)",\s*(0x[0-9a-f]+),\s*"([0-9a-f]+)"',(ROOT/'src'/header).read_text()):
  assert int(at,16)==address(n) and prefix==entries[n]['prefix'],(header,n)
# Check the shipping byte guards as well as the address inventory.
text=(ROOT/'src/spectator_contract.h').read_text()
for label,at in [('party',address('party_getter')),('remote',address('remote_getter')),('render',address('render')),('transform',slots['party_transform_return']-17)]:
 data=bytes.fromhex(re.search(label+r'_signature="([0-9a-f]+)',text)[1]);assert im.pe.get_data(at,len(data))==data,'spectator '+label
for file,label,at in [('stock_hid_contract.h','stock_hid_prefix',address('hid')),('native_camera_interop.cpp','prefix',address('sampler')),('stock_bridge.cpp','console_prefix',address('console'))]:
 text=(ROOT/'src'/file).read_text()
 values=re.search(label+r'\{([^}]+)',text)[1]
 data=bytes(int(x.strip(),0) for x in values.split(',') if x.strip());assert im.pe.get_data(at,len(data))==data,file

# Independently compiled /MT, iterator-debug=0 probe against release source.
def layout(path):return dict(line.split('=',1) for line in path.read_text(encoding='utf-8-sig').splitlines() if '=' in line)
before=layout(ROOT/'tools/migration213/layout.txt');after=layout(folder/'layout.txt')
changed={'O.context':'2912','O.menu':'29104','O.window':'728','O.callbacks':'2920','O.model':'2968'}
for key,value in before.items():assert after[key]==changed.get(key,value),('layout',key)
files=['External/imgui/imgui.h','External/imgui/imgui_internal.h','External/imgui/imconfig.h','Engine/Game/Input/playstation_report.h']
for f in files:assert (ROOT/'vendor'/f).read_bytes()==(a.upstream/f).read_bytes(),f
report={'profile':'animator-studio-1.0.101','studio_version':'1.0.11+reskate.2.1.3','source_revision':'ce058bfd5b7a3d28094676e70bb05454276a77f3','dll_sha256':im.digest,'function_prefixes':len(entries),'layout_fields':len(after),'changed_offsets':changed,'relationships':'PASS','disabled_features':['MSC presence announcements','automatic updater'],'live_gameplay':'pending'}
(folder/'audit-report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))

