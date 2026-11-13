"""Read-only exact-build evidence check; does not launch the game or patch a DLL."""
from pathlib import Path
import hashlib, struct, sys
import pefile
path = Path(sys.argv[1]) if len(sys.argv)>1 else Path(__file__).resolve().parents[1]/'work/ttr-latest-7eb72860/TTWVR.dll'
raw=path.read_bytes()
assert hashlib.sha256(raw).hexdigest()=='7eb72860d209d9b9bdd774607220ee63323367aa9fa525f6eb1b23184f30f91e'
p=pefile.PE(data=raw)
assert p.FILE_HEADER.Machine==0x14c and p.FILE_HEADER.TimeDateStamp==0x6af60df2
assert p.OPTIONAL_HEADER.Magic==0x10b and p.OPTIONAL_HEADER.ImageBase==0x10000000
checks={
0x34dd22:'80bdafe8ffff00', 0x34e861:'80bd3be9ffff00',
0x34e90f:'ff35803e8b11a3643f8b11e861d4fcff',
0x350852:'e849aefcffff359c3c8b113bf00f4cc6a3403e8b11',
0x320320:'e9d840cfff', 0x315cf0:'ff742404', 0x41c350:'8b4c2404',
0x328c40:'83ec2456570f57c0', 0x41a910:'558bec6afe',
0x31b6a0:'83ec08535657', 0x1a95a0:'81eccc0000005657',
0x32ec00:'81eccc000000', 0x31b9b0:'83ec24568b74242c'}
for rva,value in checks.items():
    expected=bytes.fromhex(value)
    assert p.get_data(rva,len(expected))==expected,hex(rva)
for rva,target in [(0x34e91a,0x31bd80),(0x350852,0x31b6a0),(0x34ced3,0x328c40),(0x3453b8,0x32ec00),(0x3499a9,0x31b9b0)]:
    data=p.get_data(rva,5)
    assert data[0]==0xe8 and rva+5+struct.unpack_from('<i',data,1)[0]==target
print('PASS: exact SHA256/PE32, both original CMPs, both native destinations and helper/call guards')
