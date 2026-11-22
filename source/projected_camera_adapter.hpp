#pragma once
// FalloutNV 1.4.0.525 / verified TTR, installed only after build and byte guards.
namespace projected_lighting {
// TTR updates eye transform/frustum before native draws; NiCamera+9C is
// still the previous eye's cached matrix here. Build the current camera from
// the live orthonormal basis and asymmetric frustum instead.
inline bool camera(float* inv,float* eye){
 __try{auto graph=*reinterpret_cast<unsigned char**>(0x11DEB7C);auto cam=graph?*reinterpret_cast<unsigned char**>(graph+0xAC):nullptr;
  if(!cam)return false;const float* basis=reinterpret_cast<float*>(cam+0x68);const float* f=reinterpret_cast<float*>(cam+0xDC);
  memcpy(eye,cam+0x8C,12);eye[3]=0;
  float width=f[1]-f[0],height=f[2]-f[3],nearZ=f[4],farZ=f[5];
  if(!(width>.01f&&height>.01f&&nearZ>0&&farZ>nearZ))return false;
  float clip[16]{};
  for(int j=0;j<3;j++){
   clip[j]=2/width*basis[j*3+2]-(f[1]+f[0])/width*basis[j*3];
   clip[4+j]=2/height*basis[j*3+1]-(f[2]+f[3])/height*basis[j*3];
   clip[8+j]=farZ/(farZ-nearZ)*basis[j*3];clip[12+j]=basis[j*3];
  }
  // Reconstruct relative to the eye to preserve precision at distant cells.
  clip[11]-=nearZ*farZ/(farZ-nearZ);
  return inverse(clip,inv);
 }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
}
