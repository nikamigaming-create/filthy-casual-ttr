from pathlib import Path
import json,struct,math,hashlib,io,shutil
import numpy as np
from PIL import Image,ImageDraw,ImageFont
r=Path(__file__).resolve().parents[1]; src=r/'work/controller-profiles/downloads';dest=r/'assets/controls'
def matrix(n):
 if 'matrix' in n:return np.array(n['matrix']).reshape(4,4).T
 x,y,z,w=n.get('rotation',[0,0,0,1]);m=np.eye(4);m[:3,:3]=[[1-2*(y*y+z*z),2*(x*y-z*w),2*(x*z+y*w)],[2*(x*y+z*w),1-2*(x*x+z*z),2*(y*z-x*w)],[2*(x*z-y*w),2*(y*z+x*w),1-2*(x*x+y*y)]];m[:3,:3]=m[:3,:3]@np.diag(n.get('scale',[1,1,1]));m[:3,3]=n.get('translation',[0,0,0]);return m
def mask(name,hand):
 n=name.lower();h=hand=='right'
 if 'thumbrest' in n:return 1<<(20 if h else 19)
 if 'thumbstick' in n or 'joystick' in n:return (1<<(6 if h else 5))|(1<<(26 if h else 25))|(15<<(15 if h else 11))
 if 'touchpad' in n or 'trackpad' in n:return 1<<(31 if h else 30)
 if 'trigger' in n:return (1<<(10 if h else 9))|(1<<(28 if h else 27))
 if 'squeeze' in n or 'grip_button' in n:return 1<<(8 if h else 7)
 if 'menu' in n or 'view_button' in n:return 1<<(29 if h else 4)
 for key,bit in [('a_button',0 if h else 2),('b_button',1 if h else 3),('x_button',2),('y_button',3)]:
  if key in n or key.replace('_','-') in n:return 1<<bit
 return 0
def write_model(folder,hand,parts,image):
 folder.mkdir(exist_ok=True,parents=True);buf=bytearray(b'M5CT'+struct.pack('<II',2,len(parts)))
 for m,vertices,indices in parts:
  assert 0<len(vertices)<=250000 and 0<len(indices)<=250000
  buf+=struct.pack('<QII',m,len(vertices),len(indices));buf+=np.array(vertices,dtype='<f4').tobytes();buf+=np.array(indices,dtype='<u4').tobytes()
 (folder/f'{hand}.mesh').write_bytes(buf);image.save(folder/f'{hand}.png')
records=[]
for folder in src.iterdir():
 profile=json.loads((folder/'profile.json').read_text())
 for hand in ['left','right']:
  asset=folder/profile['layouts'][hand]['assetPath'];data=asset.read_bytes();size=struct.unpack_from('<I',data,12)[0];doc=json.loads(data[20:20+size]);binary=data[28+size:];nodes=doc['nodes'];world={};anc={}
  def visit(i,parent,names):
   n=nodes[i];world[i]=parent@matrix(n);anc[i]=[n.get('name','')]+names
   for c in n.get('children',[]):visit(c,world[i],anc[i])
  for i in doc['scenes'][doc.get('scene',0)]['nodes']:visit(i,np.eye(4),[])
  def access(i):
   a=doc['accessors'][i];v=doc['bufferViews'][a['bufferView']];dtype={5126:'<f4',5125:'<u4',5123:'<u2',5121:'u1'}[a['componentType']];count={'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4}[a['type']];offset=v.get('byteOffset',0)+a.get('byteOffset',0);dt=np.dtype(dtype);stride=v.get('byteStride',dt.itemsize*count);return np.ndarray((a['count'],count),dtype=dt,buffer=binary,offset=offset,strides=(stride,dt.itemsize)).copy()
  mats=doc.get('materials',[{}]);tiles=[]
  for mat in mats:
   pbr=mat.get('pbrMetallicRoughness',{});factor=pbr.get('baseColorFactor',[1,1,1,1]);tex=pbr.get('baseColorTexture')
   if tex is not None:
    view=doc['bufferViews'][doc['images'][doc['textures'][tex['index']]['source']]['bufferView']];start=view.get('byteOffset',0);im=Image.open(io.BytesIO(binary[start:start+view['byteLength']])).convert('RGBA').resize((512,512))
   else:im=Image.new('RGBA',(512,512),'white')
   a=np.array(im).astype(float);a*=np.array(factor);tiles.append(Image.fromarray(np.clip(a,0,255).astype('uint8')))
  assert len(tiles)<=8
  atlas=Image.new('RGBA',(512*len(tiles),512));[atlas.paste(im,(512*i,0)) for i,im in enumerate(tiles)]
  geom=[]
  for i,m in world.items():
   if 'mesh' not in nodes[i]:continue
   # Animation target/min/max nodes are not visible geometry. Mesh nodes in the
   # active scene already hold the default pose, as authored by each manufacturer.
   for primitive in doc['meshes'][nodes[i]['mesh']]['primitives']:
    assert primitive.get('mode',4)==4
    a=primitive['attributes'];positions=access(a['POSITION']);normals=access(a['NORMAL']) if 'NORMAL' in a else np.tile([0,1,0],(len(positions),1));uv=access(a['TEXCOORD_0']) if 'TEXCOORD_0' in a else np.zeros((len(positions),2));indices=access(primitive['indices']).flatten() if 'indices' in primitive else np.arange(len(positions))
    positions=(m@np.c_[positions,np.ones(len(positions))].T).T[:,:3];normals=(np.linalg.inv(m[:3,:3]).T@normals.T).T;normals/=np.maximum(np.linalg.norm(normals,axis=1)[:,None],1e-8)
    uv[:,0]=(uv[:,0]+primitive.get('material',0))/len(tiles)
    name=next((n for n in anc[i] if mask(n,hand)),nodes[i].get('name',''))
    geom.append((mask(name,hand),positions,normals,uv,indices))
  allp=np.concatenate([g[1] for g in geom]);center=(allp.min(0)+allp.max(0))/2
  front=np.array([0,.80,-.60]);right=np.array([1,0,0]);up=np.cross(front,right);rot=np.array([right,up,front]);rotated=(rot@(allp-center).T).T;scale=.145/max(np.ptp(rotated,axis=0)[1],.01)
  parts=[]
  for flags,pos,norm,uv,indices in geom:parts.append((flags,np.c_[((rot@(pos-center).T).T)*scale,(rot@norm.T).T,uv],indices))
  write_model(dest/folder.name,hand,parts,atlas)
  records.append({'controller':folder.name,'hand':hand,'url':'https://cdn.jsdelivr.net/npm/@webxr-input-profiles/assets@1.0.20/dist/profiles/'+folder.name+'/'+asset.name,'sha256':hashlib.sha256(data).hexdigest(),'license':'MIT','parts':len(parts),'vertices':sum(len(g[1]) for g in parts)})
 # Retain upstream component metadata alongside model files.
 shutil.copy2(folder/'profile.json',dest/folder.name/'profile.json')
# Steam Frame: original schematic, with actual button layout and separate parts.
# No extracted Valve assets are redistributed, and it is labelled as a diagram.
for hand in ['left','right']:
 h=hand=='right';parts=[];atlas=Image.new('RGB',(1024,256),(35,38,40));draw=ImageDraw.Draw(atlas);font=ImageFont.truetype('C:/Windows/Fonts/arialbd.ttf',80)
 labels=['','A','B','X','Y','^','v','<','>','+','-','L','R','']
 for k,l in enumerate(labels):
  color=(28,32,35) if k==0 else (75,80,78);draw.rectangle((k*64,0,(k+1)*64-1,255),fill=color)
  if l:draw.text((k*64+32,100),l,font=font,anchor='mm',fill=(210,220,205))
 def ellipsoid(center,radii,flag,tile=0,segments=24,rings=12):
  vertices=[];indices=[]
  for y in range(rings+1):
   theta=math.pi*y/rings
   for x in range(segments+1):
    phi=2*math.pi*x/segments;n=np.array([math.sin(theta)*math.cos(phi),math.cos(theta),math.sin(theta)*math.sin(phi)]);p=np.array(center)+n*np.array(radii);normal=n/np.array(radii);normal/=np.linalg.norm(normal);u=(tile*64+32+n[0]*28)/1024;v=.40-n[1]*.30;vertices.append([*p,*normal,u,v])
  for y in range(rings):
   for x in range(segments):
    a=y*(segments+1)+x;b=a+segments+1;indices +=[a,b,a+1,a+1,b,b+1]
  parts.append((flag,vertices,indices))
 ellipsoid((0,-.018,0),(.024,.057,.020),0)
 ellipsoid((0,.028,0),(.041,.035,.021),0)
 # Top and inside buttons are high enough to see alongside the stick.
 sx=-.019 if h else .019
 ellipsoid((sx,.028,.025),(.011,.011,.006),1<<(6 if h else 5))
 bx=.017 if h else -.017;by=.045
 for dx,dy,flag,tile in [(0,-.012,0 if h else 35,1 if h else 6),(.012,0,1 if h else 37,2 if h else 8),(-.012,0,38 if h else 36,3 if h else 7),(0,.012,39 if h else 34,4 if h else 5)]:ellipsoid((bx+dx,by+dy,.022),(.005,.005,.0035),1<<flag,tile)
 ellipsoid((0,.008,.023),(.005,.004,.002),1<<(29 if h else 4),9 if h else 10)
 ellipsoid((0,.063,.002),(.022,.007,.012),1<<(33 if h else 32),12 if h else 11)
 ellipsoid((0,.042,-.020),(.014,.014,.008),1<<(10 if h else 9))
 ellipsoid((-.021 if h else .021,-.015,.002),(.005,.020,.012),1<<(8 if h else 7))
 write_model(dest/'steam-frame',hand,parts,atlas)
records.append({'controller':'steam-frame','source':'Original Filthy Casuals schematic based on Valve published button layout. Not a manufacturer mesh.','license':'project license','reference':'https://github.com/ValveSoftware/Unity/blob/main/com.valvesoftware.openxr.utils/Documentation~/index.md'})
shutil.copy2(r/'work/controller-profiles/LICENSE-webxr.md',dest/'LICENSE-webxr-controllers.md')
(dest/'controller-models-provenance.json').write_text(json.dumps(records,indent=2)+'\n')
print('Packed',len(records),'controller model provenance records')
