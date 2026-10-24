"""Read-only audit of the companion MSC bridge against official ReSkate 2.1.1.
Server protocol pinned to Jish2637/ReSkateMerge main 7a7ed9b7facfef4c89af09d6b16d99c3ff07d875:
Server/msc_presence.h, Server/steam_server.cpp. Session fields are asserted by
layout211/layout211.cpp against the official db99ed4d source.
"""
import argparse
from pathlib import Path
from inspect_hooks import Image

def audit(path):
    image=Image(path)
    assert image.digest=="d79ff144768ce8b0a8e992c9f134c670a3ff85bb3b437a0eb915876973014301"
    entries={
        0xa14590:"405356574154415541564157b8f01200",
        0x979eb0:"488bc45341574883ec58488968084d8b",
        0x9efc10:"48895c24205556574154415541564157",
        0x9c98c7:"488b054a8add00",
    }
    for address,hex_bytes in entries.items():
        expected=bytes.fromhex(hex_bytes)
        assert image.pe.get_data(address,len(expected))==expected,hex(address)
    assert 0x9c98ce+int.from_bytes(image.pe.get_data(0x9c98ca,4),'little',signed=True)==0x17a2318
    assert any(i.mnemonic=='call' and i.op_str=='0xa21e80' for i in image.code(0x979eb0,0xe0))
    assert any(i.mnemonic=='call' and i.op_str=='0x9c98a0' for i in image.code(0xa14590,0x250))
    print('MSC native 2.1.1 image identity, queue/dispatch/sender ABI and session pointer PASS')

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--dll',type=Path,required=True)
    audit(parser.parse_args().dll)
