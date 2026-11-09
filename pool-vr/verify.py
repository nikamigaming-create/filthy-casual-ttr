"""Validate the locally generated pool plugin and animation references."""
from pathlib import Path
import json,struct,hashlib
ROOT=Path(__file__).resolve().parents[1]
MOD=ROOT/'build/pool-vr/mod'
bank=json.loads((ROOT/'build/pool-vr/pose-bank.json').read_text())
assert len(bank)==351 and len({p['form'] for p in bank})==351
blob=(MOD/'Filthy Casual Pool.esp').read_bytes();records=[]
def walk(lo,hi):
 p=lo
 while p<hi:
  tag,size=struct.unpack_from('<4sI',blob,p)
  if tag==b'GRUP':
   assert size>=24 and p+size<=hi
   walk(p+24,p+size);p+=size;continue
  assert p+24+size<=hi
  flags,form=struct.unpack_from('<II',blob,p+8)
  assert tag==b'TES4' or form>>24==1, 'Vanilla override in addon'
  records.append((tag,form));q=p+24
  while q<p+24+size:
   t,n=struct.unpack_from('<4sH',blob,q);q+=6;data=blob[q:q+n];q+=n
   if t==b'MODL':assert (MOD/'meshes'/data.rstrip(b'\0').decode().replace('\\','/')).is_file(),data
  assert q==p+24+size;p+=24+size
 assert p==hi
walk(0,len(blob))
assert len(records)==357 and len({form for _,form in records})==357
assert sum(tag==b'IDLE' for tag,_ in records)==352
for item in bank:
 path=MOD/'meshes'/item['path'].replace('\\','/')
 assert path.is_file() and path.stat().st_size>1000
result={'records':len(records)-1,'poseVariants':len(bank),'masterOverrides':0,
 'espSha256':hashlib.sha256(blob).hexdigest(),
 'source':'Owned local skeleton, newly authored animation keys; upstream installed separately'}
(ROOT/'evidence/pool-package-validation.json').write_text(json.dumps(result,indent=2))
print(json.dumps(result))
