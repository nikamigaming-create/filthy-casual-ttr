"""Private canopy fit study; retains source silhouette/UVs and changes only panels.
Output contains locally owned data. It is neither a release nor a runtime proof.
"""
import json,math,sys
from collections import defaultdict
import numpy as np
from pathlib import Path
root=Path(sys.argv[1]);g=json.loads((root/'geometry.json').read_text());out=[];parts=[]
# Source coordinates: nose faces -Y; roof rises rearward. Solid center/cross ribs.
from window_contract import PLANES as planes,split,inside
for m in g['meshes']:
 if m['group']!='exterior':continue
 solid={**m,'vertices':[],'uvs':[],'triangles':[]};glass={**solid,'name':m['name']+' canopy glazing','vertices':[],'uvs':[],'triangles':[],'diffuse':None,'normal':None,'glass':True}
 for tri in m['triangles']:
  p=[m['vertices'][i]+m['uvs'][i] for i in tri]
  u=[p[1][i]-p[0][i] for i in range(3)];v=[p[2][i]-p[0][i] for i in range(3)];normal=[u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0]];length=math.sqrt(sum(x*x for x in normal))
  candidate=length>1e-8 and min(q[1] for q in p)<-185 and max(q[1] for q in p)>-325 and min(abs(q[0]) for q in p)<90 and max(q[2] for q in p)>210
  polys=[p]
  if candidate:
   for axis,value in planes:polys=[part for poly in polys for part in split(poly,axis,value)]
  for poly in polys:
   x,y,z=[sum(p[i] for p in poly)/len(poly) for i in range(3)]
   glazing=candidate and inside(x,y,z)
   # Remove inward-facing legacy window returns from the new opening too.
   # Only the exterior roof surface supplies the new glazing.
   if glazing and normal[2]/length<=0:continue
   dst=glass if glazing else solid;n=len(dst['vertices']);dst['vertices'].extend(p[:3] for p in poly);dst['uvs'].extend(p[3:] for p in poly);dst['triangles'].extend([n,n+i,n+i+1] for i in range(1,len(poly)-1))

 parts.append((m,solid,glass))

# Material boundaries are not physical seams. Use a single welded normal field
# across both hull sections, so their inward-offset vertices remain coincident.
def key(v):return tuple(round(c,4) for c in v)
normals=defaultdict(lambda:np.zeros(3))
for m,solid,glass in parts:
 for part in (solid,glass):
  for tri in part['triangles']:
   a,b,c=[np.array(part['vertices'][i]) for i in tri];n=np.cross(b-a,c-a)
   for i in tri:normals[key(part['vertices'][i])]+=n
for k,n in normals.items():
 length=np.linalg.norm(n);normals[k]=n/length if length>1e-8 else np.array([0,0,1.])
for m,solid,glass in parts:
 liner={**m,'name':m['name']+' inner canopy frame','vertices':[],'uvs':[],'triangles':[]}
 depth=1.4
 def face(dst,points,uvs):
  base=len(dst['vertices']);dst['vertices'].extend([list(map(float,p)) for p in points]);dst['uvs'].extend([list(map(float,u)) for u in uvs]);dst['triangles'].extend([base,base+j,base+j+1] for j in range(1,len(points)-1))
 for tri in solid['triangles']:
  p=[np.array(solid['vertices'][i]) for i in tri];x,y,z=np.mean(p,axis=0);normal=np.cross(p[1]-p[0],p[2]-p[0])
  if np.linalg.norm(normal)>1e-8:
   face(liner,[p[j]-normals[key(p[j])]*depth for j in (2,1,0)],[solid['uvs'][tri[j]] for j in (2,1,0)])
 edges={};counts=defaultdict(int)
 for tri in glass['triangles']:
  for a,b in zip(tri,tri[1:]+tri[:1]):
   k=tuple(sorted((key(glass['vertices'][a]),key(glass['vertices'][b]))));counts[k]+=1;edges[k]=(a,b)
 jambs=0
 for k,count in counts.items():
  if count!=1:continue
  a,b=edges[k];p,q=np.array(glass['vertices'][a]),np.array(glass['vertices'][b]);n0,n1=normals[key(p)],normals[key(q)];uv0,uv1=np.array(glass['uvs'][a]),np.array(glass['uvs'][b]);duv=uv1-uv0;ln=np.linalg.norm(duv);uvside=np.array([-duv[1],duv[0]])/ln*.006 if ln>1e-8 else np.array([.006,0])
  # Depth segments give the metal aperture an actual inner edge.
  for t0,t1 in [(0,.25),(.25,1.)]:
   face(liner,[p-n0*depth*t0,q-n1*depth*t0,q-n1*depth*t1,p-n0*depth*t1],[uv0+uvside*t0,uv1+uvside*t0,uv1+uvside*t1,uv0+uvside*t1])
  jambs+=1
 glass['vertices']=[(np.array(v)-normals[key(v)]*.3).tolist() for v in glass['vertices']]
 out.extend([solid,glass,liner]);print('closed aperture edges',jambs,'liner triangles',len(liner['triangles']))
g['meshes']=out;g['design']={'status':'private fitted canopy with inner skin and aperture jambs','solid_center_rib_units':8,'solid_cross_rib_units':7,'source_silhouette_preserved':True,'source_textures_preserved':True,'eye_position_provisional':[-42,-285,270]}
(root/'canopy-geometry.json').write_text(json.dumps(g,separators=(',',':')),encoding='utf8')
print([(m['name'],len(m['triangles'])) for m in out])
