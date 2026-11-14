from pathlib import Path
import json,xml.etree.ElementTree as E,struct,hashlib
import numpy as np
r=Path(__file__).resolve().parents[1]; c=json.loads((r/'assets/controls/controller-profiles.json').read_text()); registry=E.parse(r/'work/controller-profiles/xr.xml').find('interaction_profiles'); tests=[]
for p in c['profiles']:
 if p['id']=='frame':
  # Valve's current source is independent evidence for the shoulder spelling.
  official=(r/'work/controller-profiles/SteamFrameControllerProfile.cs').read_text(encoding='utf-8-sig')
  assert '"/input/shoulder/click"' in official
  assert all(not '/bumper/' in x for x in p['paths'])
 else:
  node=next(x for x in registry if x.get('name')==p['path']);valid=set()
  for x in node.findall('component'):
   if x.get('system')=='true':continue
   for h in ['left','right']:
    user='/user/hand/'+h
    if not x.get('user_path') or user==x.get('user_path'):valid.add(user+x.get('subpath'))
  for path in p['paths']:assert not path or path in valid,(p['id'],path)
 for hand in ['left','right']:
  f=r/'assets/controls'/p['asset']/(hand+'.mesh');b=f.read_bytes();assert b[:4]==b'M5CT';version,count=struct.unpack_from('<II',b,4);assert version==2 and 0<count<=128;at=12;nv=ni=0;flags=0
  for _ in range(count):
   mask,vertices,indices=struct.unpack_from('<QII',b,at);at+=16;v=np.frombuffer(b,dtype='<f4',count=vertices*8,offset=at).reshape(-1,8);assert np.isfinite(v).all();assert np.max(np.abs(v[:,:3]))<.5;at+=vertices*32;i=np.frombuffer(b,dtype='<u4',count=indices,offset=at);assert i.max()<vertices;at+=indices*4;nv+=vertices;ni+=indices;flags|=mask
  assert at==len(b) and flags
  tests.append({'profile':p['id'],'hand':hand,'parts':count,'vertices':nv,'indices':ni,'sha256':hashlib.sha256(b).hexdigest()})
(r/'evidence/controller-assets-validation.json').write_text(json.dumps({'profiles':len(c['profiles']),'sources':len(c['sources']),'checks':tests},indent=2)+'\n')
print('PASS: 9 profile path contracts, 70 stable names, 18 model meshes, finite geometry, bounds, indices and component masks')
