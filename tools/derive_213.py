"""Derive candidate 2.1.3 RVAs from the reviewed 2.1.2 binary; offline only."""
import sys, pathlib, json
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT.parent/'.trace-deps'))
from inspect_hooks import Image, signature, source_initializer, destination, rip, calls
old=Image(ROOT.parent/'upstream/ReSkate-2.1.2b.dll')
new=Image(ROOT.parent/'upstream/ReSkate-2.1.3.dll')
out=ROOT/'tools/migration213'
out.mkdir(exist_ok=True)
entries=json.loads((ROOT/'tools/migration212b/contracts.json').read_text())
result={}
for name,e in entries.items():
    try:
        rva=signature(old,new,e['rva'], min(len(bytes.fromhex(e['prefix'])),32) if name=='context' else 96)
        result[name]={'old':e['rva'],'rva':rva,'prefix':new.pe.get_data(rva,len(bytes.fromhex(e['prefix']))).hex()}
        print(name,hex(e['rva']),'->',hex(rva),flush=True)
    except Exception as ex: print('UNRESOLVED',name,str(ex),flush=True)
(out/'contracts.json').write_text(json.dumps(result,indent=2)+'\n')
print('status export',hex(new.export('DingoSDKOverlayGetStatus')))
