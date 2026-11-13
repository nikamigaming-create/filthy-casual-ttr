"""One geometric aperture contract for exterior glass and original cabin trim."""
PLANES=[(0,-72),(0,-4),(0,4),(0,72),(1,-325),(1,-230),(1,-223),(1,-185),(2,210),(3,425)]
def inside(x,y,z):
 return 4<abs(x)<72 and ((-325<y<-230) or (-223<y<-185)) and z>210 and z>.65*y+425
def split(poly,axis,value):
 sides=[]
 for sign in (1,-1):
  result=[]
  for i,p in enumerate(poly):
   q=poly[(i+1)%len(poly)];dp=((p[axis] if axis<3 else p[2]-.65*p[1])-value)*sign;dq=((q[axis] if axis<3 else q[2]-.65*q[1])-value)*sign
   if dp>=-1e-7:result.append(p)
   if (dp>1e-7 and dq< -1e-7) or (dp< -1e-7 and dq>1e-7):
    t=dp/(dp-dq);result.append([p[k]+t*(q[k]-p[k]) for k in range(len(p))])
  if len(result)>2:sides.append(result)
 return sides
