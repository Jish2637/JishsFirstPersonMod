"""Read-only binary/source audit for the embedded official 2.1.1 page.
Never loads the reference DLL. Requires pefile/capstone, as audit_211.py does.
"""
import argparse,hashlib,json,re,struct,zipfile
from pathlib import Path
from inspect_hooks import Image,calls,destination

def audit(dll,upstream,baseline):
 root=Path(__file__).resolve().parents[1]
 im=Image(dll)
 assert im.digest=='d79ff144768ce8b0a8e992c9f134c670a3ff85bb3b437a0eb915876973014301','unsupported reference'
 contracts=json.loads((root/'tools/migration211/embedded-contracts.json').read_text())
 for name,v in contracts.items():
  size=len(bytes.fromhex(v['prefix']))
  assert im.section(v['rva'],size,0x20000000),name
  assert im.pe.get_data(v['rva'],size).hex()==v['prefix'],name
 # Derive identity from page string references AND the skater tab dispatch.
 code=im.code(contracts['draw']['rva'],0xd00)
 strings={im.pe.get_data(destination(i),96).split(b'\0')[0] for i in code if i.mnemonic=='lea' and '[rip' in i.op_str}
 assert {b'first-person',b'FIRST PERSON',b'Field of view',b'Spring arm'}.issubset(strings)
 page_calls=calls(im.code(0xe54f80,0x10e))
 assert 0xe537b0 in page_calls,'First Person tab dispatch'
 expected_calls={contracts[n]['rva'] for n in ['begin_card','end_card','field','note','slider_float','toggle_row']}
 assert expected_calls.issubset(set(calls(code))),'page/widget call graph'
 # Both allocators use the same host context/user data and host-owned callback slots.
 alloc=im.code(0xfd12f0,0xa9);free=im.code(0xfd13a0,0x86)
 refs=lambda code:{destination(i) for i in code if '[rip' in i.op_str}
 assert {0x1771180,0x18fcc40,0x18fcc38}.issubset(refs(alloc)),'host allocation slots'
 assert {0x1771188,0x18fcc40,0x18fcc38}.issubset(refs(free)),'host release slots'
 assert destination(im.code(0xfcc5b0,7)[0])==0x18fcc38,'GetCurrentContext slot'
 imagebase=im.pe.OPTIONAL_HEADER.ImageBase
 for slot in [0x1771180,0x1771188]:
  callback=struct.unpack('<Q',im.pe.get_data(slot,8))[0]-imagebase
  assert im.section(callback,1,0x20000000),'default allocator is not executable'
 # Core/config declarations must match before sharing any host ImGui state.
 normalize=lambda b:b.replace(b'\r\n',b'\n')
 for f in ['imconfig.h','imgui.h','imgui_internal.h','imgui.cpp','imgui_draw.cpp','imgui_widgets.cpp','imgui_tables.cpp']:
  assert normalize((root/'vendor/External/imgui'/f).read_bytes())==normalize((upstream/'External/imgui'/f).read_bytes()),'ImGui mismatch: '+f
 # Every original src file (camera, input, settings, preset/storage algorithms) is identical.
 with zipfile.ZipFile(baseline) as z:
  files=[n for n in z.namelist() if n.startswith(('src/','vendor/')) and not n.endswith('/')]
  for n in files:assert (root/n).read_bytes()==z.read(n),'baseline modified: '+n
 cmake=(root/'CMakeLists.txt').read_text()
 shipping=cmake.split('add_library(JishsFirstPersonMod SHARED',1)[1].split('include(CTest)',1)[0]
 assert 'src/embedded_runtime.cpp' in shipping and 'PRIVATE mod_embedded_ui ' in shipping
 assert all(n not in shipping for n in ['src/menu_renderer.cpp','src/menu_input.cpp','mod_menu_ui ']),'duplicate presentation/input owner'
 active=['embedded_runtime.cpp','embedded_menu.cpp','recording_input.cpp']
 for n in active:
  text=(root/'src'/n).read_text()
  assert not re.search(r'VK_F9|\bF9\b|ImGui::(?:NewFrame|Render|CreateContext|DestroyContext)|CreateWindow|D3D12CreateDevice|ExecuteCommandLists',text),n+' owns frame/shortcut/graphics'
 runtime=(root/'src/embedded_runtime.cpp').read_text()
 worker=runtime.split('void polling_step()',1)[1].split('\n}\n}',1)[0]
 assert 'poll_stock_camera();' in worker and 'Sleep(50)' in worker and 'ImGui::' not in worker,'closed-menu polling'
 assert 'original_draw(menu,model,callbacks)' in runtime and 'No companion renderer' in runtime,'contract rejection fallback'
 assert not any('MSC-FP' in (root/'src'/n).read_text() or '/jish-fp' in (root/'src'/n).read_text() for n in active)
 return {'official_dll_sha256':im.digest,'verified_entries':len(contracts),'page_dispatch_widget_allocator_relationships':'PASS','imgui_core_and_config':'MATCH','baseline_src_and_vendor_files_unchanged':len(files),'shipping_frame_and_F9_ownership':'STOCK','closed_Insert_polling':'50 ms worker, guarded original poll','result':'PASS'}
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--dll',type=Path,required=True);p.add_argument('--upstream',type=Path,required=True);p.add_argument('--baseline',type=Path,required=True);a=p.parse_args()
 print(json.dumps(audit(a.dll,a.upstream,a.baseline),indent=2))
