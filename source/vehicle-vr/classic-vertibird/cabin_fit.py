"""Fit locally owned cabin lining to the uncut classic hull, preserving source UVs."""
import numpy as np
from window_contract import PLANES,split

def canopy_region(x,y,z):
 # The exterior-derived closed frame replaces both old windows AND old ribs.
 return abs(x)<72 and -325<y<-185 and z>210 and z>.65*y+425
def cabin_depth(points):
 # The dependency cabin is longer than the classic nose. Preserve the seats and
 # footwell through Y=160; smoothly shorten only the forward instrument bay.
 # Positive derivative .5..1 keeps source detail ordered instead of folding it.
 out=np.array(points,copy=True);q=np.maximum(out[...,1]-160,0);r=np.where(q<20,q*q/40,q-10);out[...,1]-=.5*r;return out
class CabinFit:
 def __init__(self,exterior,interior,R,T):
  self.R,self.T=R,T
  self.hull=self.prepare(exterior);self.stats=[]
  points=np.unique(np.round(cabin_depth(interior).reshape(-1,3),5),axis=0)
  self.grid=np.arange(np.floor(points[:,1].min()/5)*5-5,np.ceil(points[:,1].max()/5)*5+10,5.)
  factors=np.ones(len(self.grid))
  for v in points:
   if v[2]<=515:continue
   o=np.array([0,v[1],486.9]);d=v-o;r=np.linalg.norm(d);h=self.hits(self.hull,o,d/r)
   if len(h):
    i=int(np.clip(np.floor((v[1]-self.grid[0])/5),0,len(factors)-1));factors[i]=min(factors[i],(max(h)-2.5)/r)
  assert min(factors)>.3, ('Cabin does not fit this source hull',min(factors))
  # A conservative shared cross-section cage keeps thin source details intact.
  # Positive scale and unchanged Y guarantee positive volume Jacobian; unlike
  # vertex shrinkwrap, adjacent layers never collapse onto the same surface.
  self.factors=np.array([min(factors[max(0,i-2):min(len(factors),i+3)]) for i in range(len(factors))])
  slopes=np.diff(self.factors)/5;self.knots=self.grid[1:-1][np.abs(np.diff(slopes))>1e-9]
 def prepare(self,tri):
  return tri[:,0],tri[:,1]-tri[:,0],tri[:,2]-tri[:,0]
 def hits(self,mesh,o,d):
  a,e1,e2=mesh;h=np.cross(np.broadcast_to(d,e2.shape),e2);det=np.einsum('ij,ij->i',e1,h);safe=np.abs(det)>1e-7
  f=np.divide(1.,det,out=np.zeros_like(det),where=safe);s=o-a;u=f*np.einsum('ij,ij->i',s,h);q=np.cross(s,e1);v=f*(q@d);t=f*np.einsum('ij,ij->i',e2,q)
  return t[safe&(u>=-1e-5)&(v>=-1e-5)&(u+v<=1.00001)&(t>0)]
 def point(self,v):
  scale=float(np.interp(v[1],self.grid,self.factors))
  return np.array([v[0]*scale,v[1],486.9+(v[2]-486.9)*scale])
 def deform(self,vertices):return np.array([self.point(v) for v in cabin_depth(vertices)])
 def fit(self,name,vertices,triangles,uvs,cut_windows=True):
  vertices=cabin_depth(vertices);original=vertices.copy();fitted=np.array([self.point(v) for v in vertices])
  vv=[];tt=[];uu=[];collapsed=turned=inverted=0;bad=[]
  for tri in triangles:
   source_poly=[list(original[i])+list(uvs[i]) for i in tri];source_polys=[source_poly]
   # Split at cage section changes before deforming. Thin bolt/rib triangles
   # spanning a cage knot otherwise chord across the bend and can turn inside out.
   lo=min(p[1] for p in source_poly);hi=max(p[1] for p in source_poly)
   for y in self.knots[(self.knots>lo+1e-7)&(self.knots<hi-1e-7)]:source_polys=[part for poly in source_polys for part in split(poly,1,y)]
   polys=[]
   for source_poly in source_polys:
    poly=[list((self.point(np.array(p[:3]))-self.T)@self.R)+p[3:]+p[:3] for p in source_poly];parts=[poly]
    if cut_windows and min(p[1] for p in poly)<-185 and max(p[1] for p in poly)>-325 and max(p[2] for p in poly)>210:
     for axis,value in PLANES:parts=[part for poly in parts for part in split(poly,axis,value)]
    polys.extend(parts)
   for poly in polys:
    if cut_windows and canopy_region(*np.mean([p[:3] for p in poly],axis=0)):continue
    points=np.array([p[:3] for p in poly])@self.R.T+self.T
    base=len(vv);vv.extend(points);uu.extend(p[3:5] for p in poly)
    for j in range(1,len(poly)-1):
     ids=[0,j,j+1];src=np.array([poly[k][5:] for k in ids]);dst=points[ids]
     a=np.cross(src[1]-src[0],src[2]-src[0]);b=np.cross(dst[1]-dst[0],dst[2]-dst[0]);la=np.linalg.norm(a);lb=np.linalg.norm(b)
     if lb<1e-7:continue
     if la>1e-6 and lb<la*.01:collapsed+=1;bad.append(dict(kind="collapse",src=src.tolist(),dst=dst.tolist(),ratio=float(lb/la)))
     center=np.mean(src,axis=0);idx=int(np.clip(np.searchsorted(self.grid,center[1])-1,0,len(self.grid)-2));slope=(self.factors[idx+1]-self.factors[idx])/5;scale=float(np.interp(center[1],self.grid,self.factors))
     expected=np.array([scale*a[0],scale*scale*a[1]-scale*slope*(center[0]*a[0]+(center[2]-486.9)*a[2]),scale*a[2]])
     if la>1e-6 and np.dot(expected,b)<0:inverted+=1;bad.append(dict(kind="inverted",src=src.tolist(),dst=dst.tolist(),ratio=float(lb/la)))
     if la>1e-6 and np.dot(a,b)<0:turned+=1
     tt.append([base+k for k in ids])
  caps=0
  if cut_windows and tt:
   # Close only small cut tube/post loops at the new canopy boundary. Larger
   # cabin surfaces remain open windows, never a fabricated wall over the glass.
   from collections import defaultdict
   edges=defaultdict(list);positions={}
   def key(i):return tuple(np.round(vv[i],4))
   for tri in tt:
    for a,b in zip(tri,tri[1:]+tri[:1]):
     ka,kb=key(a),key(b);edges[tuple(sorted((ka,kb)))].append((a,b));positions[ka]=a;positions[kb]=b
   boundary={k:e[0] for k,e in edges.items() if len(e)==1}
   outer_planes=[(0,-72),(0,72),(1,-325),(1,-185),(2,210),(3,425)]
   used=set()
   for axis,value in outer_planes:
    eligible={}
    for k,(a,b) in boundary.items():
     if k in used:continue
     ps=(np.array([vv[a],vv[b]])-self.T)@self.R
     distance=ps[:,axis]-value if axis<3 else ps[:,2]-.65*ps[:,1]-value
     if max(abs(distance))<.001:eligible[k]=(a,b)
    adjacent=defaultdict(list)
    for k in eligible:
     for vertex in k:adjacent[vertex].append(k)
    seen=set()
    for seed in eligible:
     if seed in seen:continue
     component=set();queue=[seed]
     while queue:
      edge=queue.pop()
      if edge in component:continue
      component.add(edge);seen.add(edge)
      for vertex in edge:queue.extend(adjacent[vertex])
     vertices_set={v for e in component for v in e}
     if not 3<=len(component)<=64 or any(len(adjacent[v])!=2 for v in vertices_set):continue
     pts=np.array(list(vertices_set))
     if np.linalg.norm(np.ptp(pts,axis=0))>12:continue
     center=np.mean(pts,axis=0);uv=np.mean([uu[positions[v]] for v in vertices_set],axis=0);ci=len(vv);vv.append(center);uu.append(uv)
     for edge in component:
      a,b=eligible[edge];tt.append([b,a,ci]);used.add(edge)
     caps+=1
  self.stats.append(dict(name=name,triangles=len(tt),closed_post_ends=caps,collapsed=collapsed,turned=turned,inverted=inverted,max_displacement=float(np.max(np.linalg.norm(fitted-original,axis=1))),bad=bad))
  return np.array(vv),tt,uu
