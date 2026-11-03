"""Offline audit of every private 2.1.4 runtime interface. Never loads the DLL.
Usage: python tools/audit_214.py DLL --upstream TAG_SOURCE
Dependencies: tools/requirements.txt. Compile tools/layout214 separately first.
"""
import argparse,hashlib,json,pathlib,re,struct,sys
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT.parent/'.trace-deps'))
from inspect_hooks import Image,calls,destination,rip,signature
p=argparse.ArgumentParser();p.add_argument('dll',type=pathlib.Path);p.add_argument('--upstream',type=pathlib.Path,required=True);a=p.parse_args()
im=Image(a.dll);assert im.digest=='39195d824d775e5046480d66cb62dae0d709648f145414cb952ed2aa8afb99b3','DLL identity'
folder=ROOT/'tools/migration214';entries=json.loads((folder/'contracts.json').read_text());slots=json.loads((folder/'data-contracts.json').read_text())
def address(role):return entries[role]['rva']
def relative(at,disp,size):return at+size+struct.unpack('<i',im.pe.get_data(at+disp,4))[0]
for role,e in entries.items():
 data=bytes.fromhex(e['prefix']);assert im.section(e['rva'],len(data),0x20000000),role
 assert im.pe.get_data(e['rva'],len(data))==data,role
assert im.export('DingoSDKOverlayGetStatus')==address('status')
assert calls(im.code(address('status'),180))==[address('getter'),address('lock'),address('unlock')],'status/mutex relationship'
assert im.pe.get_data(address('lock'),3)==b'\x33\xd2\xe9','mutex lock ABI'
assert relative(address('lock')+2,1,5)==address('lock_impl'),'mutex thunk destination'
previous=Image(ROOT.parent/'upstream/ReSkate-2.1.3.dll')
assert signature(previous,im,entries['lock_impl']['old'],96)==address('lock_impl'),'recursive mutex implementation ABI'
for key in ('state','party','remote','session','context','alloc','free','userdata','source_initialized','source_base','source_transform'):
 assert im.section(slots[key],8,0xC0000000),key+' writable global'
for slot,role in [('state','getter'),('party','party_getter'),('remote','remote_getter'),('session','msc_session')]:
 assert slots[slot] in {destination(i) for i in im.code(address(role),100) if rip(i)},slot+' accessor'
assert relative(address('getter')+0x27,3,7)==slots['state']
ctor=im.code(address('ctor'),2300)
assert any(i.op_str=='qword ptr [rdi + 0xac8], rsi' for i in ctor),'callback offset'
assert any(i.op_str=='rcx, [rdi + 0xaf8]' for i in ctor),'model offset'
for role in ('mem_alloc','mem_free'):
 refs={destination(i) for i in im.code(address(role),170) if rip(i)}
 assert {slots['context'],slots['userdata'],slots['free' if role=='mem_free' else 'alloc']}<=refs,role
assert destination(im.code(address('context'),7)[0])==slots['context']
code=im.code(address('source_init'),180)
assert any('0x2f8fa90' in i.op_str for i in code),'game camera transform publication'
for key in ('source_initialized','source_base','source_transform'):
 assert slots[key] in {destination(i) for i in code if rip(i)},key
assert relative(slots['party_fov_return']-5,1,5)==address('fov'),'party FOV caller'
assert im.pe.get_data(slots['party_transform_return']-2,2)==b'\xff\xd0','party transform ABI'
for site in slots['hid_callers']:assert relative(site,1,5)==address('hid'),'HID caller'
assert len(slots['hid_callers'])==3
pad=im.code(address('gamepad'),0x400)
assert any(i.mnemonic=='movups' and i.op_str=='xmmword ptr [r14], xmm0' for i in pad),'native 16-byte pad copy'
assert any(i.mnemonic=='mov' and i.op_str=='dword ptr [r14 + 0x10], eax' for i in pad),'native pad final 4-byte copy'
# Validate the guards and literals actually used by the shipping sources.
for header in ('native_211.h','embedded_contract.h'):
 text=(ROOT/'src'/header).read_text()
 for role,rva,prefix in re.findall(r'Entry\s+\w+\{"([^"]+)",\s*(0x[0-9a-f]+),\s*"([0-9a-f]+)"',text):
  assert int(rva,16)==address(role) and prefix==entries[role]['prefix'],(header,role)
text=(ROOT/'src/msc_presence_bridge.cpp').read_text()
for rva,values in re.findall(r'bytes\(base\+(0x[0-9a-f]+),std::array<unsigned char,\d+>\{([^}]+)}',text):
 data=bytes(int(v.strip(),0) for v in values.split(','));assert im.pe.get_data(int(rva,16),len(data))==data,'MSC guard'
code=im.code(address('msc_send'),100);assert {780680,780736}<={o.mem.disp for i in code for o in i.operands if o.type==3},'MSC fields'
text=(ROOT/'src/stock_hid_contract.h').read_text()
prefix=bytes(int(x,0) for x in re.search(r'stock_hid_prefix\{([^}]+)',text)[1].split(','))
assert im.pe.get_data(address('hid'),len(prefix))==prefix,'HID guard'
text=(ROOT/'src/spectator_contract.h').read_text()
for label,rva in [('party',address('party_getter')),('remote',address('remote_getter')),('render',address('render')),('transform',slots['party_transform_return']-17)]:
 data=bytes.fromhex(re.search(label+r'_signature="([0-9a-f]+)',text)[1]);assert im.pe.get_data(rva,len(data))==data,'spectator '+label
page=im.code(address('draw'),0xd00)
strings={im.pe.get_data(destination(i),96).split(b'\0')[0] for i in page if i.mnemonic=='lea' and rip(i)}
assert {b'first-person',b'FIRST PERSON',b'Field of view',b'Spring arm'}<=strings,'page identity'
assert {address(n) for n in ('begin_card','end_card','field','note','slider_float','toggle_row')}<=set(calls(page)),'widget call graph'
assert address('draw') in calls(im.code(address('skater_page'),0x200)),'page dispatch'
# MSVC release layout probe against the exact official tag: all unchanged fields
# must still match, except the reviewed five overlay offsets and Remote stride.
def layout(path):return dict(line.split('=',1) for line in path.read_text(encoding='utf-8-sig').splitlines() if '=' in line)
before=layout(ROOT/'tools/migration213/layout.txt');after=layout(folder/'layout.txt')
changed={'O.context':'2752','O.menu':'28944','O.window':'728','O.callbacks':'2760','O.model':'2808','Remote.size':'896'}
for key,value in before.items():assert after[key]==changed.get(key,value),('layout',key,value,after.get(key))
assert after['Mutex.size']==after['RecursiveMutex.size']=='80','MSVC mutex representation'
assert after['R.mutex']=='72','remote mutex position'
assert any(i.mnemonic=='imul' and i.op_str.endswith(', 0x380') for i in im.code(address('remote_getter'),100)),'remote stride in native accessor'
text=(ROOT/'src/spectator_contract.h').read_text()
assert 'remote_stride=896' in text,'compiled spectator stride'
text=(ROOT/'src/embedded_contract.h').read_text()
assert 'state_context=2752,state_menu=28944,state_window=728' in text,'compiled overlay offsets'
text=(ROOT/'src/stock_contract.h').read_text()
assert '? 0xac8:' in text and '? 0xaf8:' in text,'compiled callbacks/model offsets'
profile=re.search(r'\{"2\.1\.4","([0-9a-f]{64})",\s*([^}]+)}',text)
assert profile and profile[1]==im.digest,'compiled native identity'
assert [int(v.strip(),0) for v in profile[2].split(',')]==[
 address('status'),slots['state'],address('lock'),address('unlock'),address('fov'),
 slots['source_initialized'],slots['source_base'],slots['source_transform']],'compiled native globals'
def constants(text):return {k:int(v,16) for k,v in re.findall(r'\b(\w+)=(0x[0-9a-f]+)',text)}
ui=constants((ROOT/'src/embedded_contract.h').read_text())
for field,slot in [('context_slot','context'),('alloc_slot','alloc'),('free_slot','free'),('userdata_slot','userdata')]:assert ui[field]==slots[slot],field
watch=constants((ROOT/'src/spectator_contract.h').read_text())
for field,expected in {'party_getter':address('party_getter'),'remote_getter':address('remote_getter'),
 'party_pointer':slots['party'],'remote_pointer':slots['remote'],'render_hook':address('render'),
 'party_transform_return':slots['party_transform_return'],'party_fov_return':slots['party_fov_return']}.items():assert watch[field]==expected,field
hid=(ROOT/'src/stock_hid_contract.h').read_text()
assert [int(v,0) for v in re.search(r'stock_hid_callers\{([^}]+)',hid)[1].split(',')]==slots['hid_callers'],'compiled HID callers'
msc=(ROOT/'src/msc_presence_bridge.cpp').read_text()
for value in (slots['session'],address('msc_command'),address('msc_queue'),address('msc_send')):assert hex(value) in msc,'compiled MSC interface'
assert relative(address('msc_session')+0x27,3,7)==slots['session'],'MSC session accessor'
sources=['Extension/UI/Overlay/overlay_internal.h','Extension/UI/Overlay/overlay.h','Extension/UI/Overlay/input_capture.cpp','Extension/UI/Overlay/overlay_input.cpp','Extension/Multiplayer/Session/session_internal.h','Extension/Multiplayer/Hud/native_party_internal.h','Extension/Multiplayer/Remote/native_skater_internal.h','External/imgui/imgui.h','External/imgui/imgui_internal.h','External/imgui/imconfig.h']
sources.insert(0,'Engine/Game/Input/playstation_report.h')
hashes={f:hashlib.sha256((a.upstream/f).read_bytes()).hexdigest() for f in sources}
assert (ROOT/'vendor'/sources[0]).read_bytes()==(a.upstream/sources[0]).read_bytes(),'2.1.4 report helper'
# Sharing the ImGui context requires identical compiled declarations/config.
for f in sources[-3:]:assert (ROOT/'vendor'/f).read_bytes()==(a.upstream/f).read_bytes(),f
for name in ('engine.h','runtime.h','camera_observer.h','client_source_spawn.h','native_skater.h'):
 f=ROOT/'vendor/Engine/Game/Build/20260929'/name
 def clean(text):return re.sub(r'\s+','',re.sub(r'/\*.*?\*/|//[^\n]*','',text,flags=re.S))
 upstream=clean((a.upstream/'Engine/Game/Build/20260929'/f.name).read_text(encoding='utf-8-sig'))
 for declaration in re.findall(r'inline\s+constexpr\s+[^;]+;',f.read_text(encoding='utf-8-sig')):
  assert clean(declaration) in upstream,f.name+' existing game contract: '+declaration
report={'revision':'919b04e','dll_sha256':im.digest,'function_prefixes':len(entries),'layout_fields':len(after),'changed_overlay_offsets':changed,'source_sha256':hashes,'relationships':'PASS'}
(folder/'audit-report.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
