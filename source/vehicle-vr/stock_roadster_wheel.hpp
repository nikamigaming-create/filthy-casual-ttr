#pragma once

// The stock 8b rims below are single-bone skins on Bip01 Spine.
namespace stock_vehicle_wheel {
struct Rim {
 const char* shapes[2];
 scene::Vec bindPosition;
 scene::Vec center;
 scene::Vec up;
 float radius;
};

inline constexpr Rim roadster{
 {"SteeringWheelA","SteeringWheelB"}, {0.f,93.77938f,-38.42404f},
 {-19.65369f,-23.72104f,82.347f}, {0.f,.3420203f,.9396926f}, 9.97413f
};
inline constexpr Rim pickup{
 {"Tri Car03.001 1","Tri Car03.001 0"}, {0.f,123.30951f,-29.44361f},
 {-24.67967f,22.67344f,91.86397f}, {0.f,.3439186f,.938998f}, 16.823f
};

inline constexpr Rim sportCoupe{
 {"SteeringWheelB","SteeringWheelA"}, {0.f,84.23110f,-25.16581f},
 {-27.51145f,32.25567f,79.88838f}, {0.f,.3420202f,.9396926f}, 10.56604f
};
inline constexpr Rim van{
 {"SteeringWheelB","SteeringWheelA"}, {0.f,97.97494f,-37.11358f},
 {-45.92859f,77.77099f,121.06783f}, {0.f,.3420202f,.9396926f}, 15.14335f
};
inline constexpr Rim firetruck{
 {"SteeringWheelB","SteeringWheelA"}, {0.f,193.40323f,-49.67456f},
 {-45.25032f,179.07338f,192.52689f}, {0.f,.3420202f,.9396926f}, 16.82596f
};
inline constexpr Rim flatbed{
 {"SteeringWheelA","SteeringWheelB"}, {0.f,247.30005f,-51.69986f},
 {-44.57937f,127.91518f,194.36076f}, {0.f,.3420202f,.9396926f}, 16.82594f
};

struct Rig {
 const Rim* rim{};
 void* root{};
 void* basis{};
 void* pivot{};
 void* driven{};
 void* original{};
 void* skins[2]{};

 bool stockRim(void* tree,const Rim& geometrySpec,scene::Transform& bind){
  const unsigned short vertexCounts[]={190,52};
  for(int i=0;i<2;++i){
   auto shape=scene::find(tree,geometrySpec.shapes[i]);if(!shape)return false;
   auto geometry=scene::field<void*>(shape,0xB8);
   if(!geometry||scene::field<unsigned short>(geometry,8)!=vertexCounts[i])return false;
   skins[i]=scene::field<void*>(shape,0xBC);if(!skins[i])return false;
   auto data=scene::field<void*>(skins[i],8);
   auto bones=scene::field<void**>(skins[i],0x14);
   if(!data||!bones||!bones[0]||scene::field<unsigned>(data,0x44)!=1)return false;
   auto info=scene::field<scene::Transform*>(data,0x40);if(!info)return false;
   if(i==0){original=bones[0];bind=*info;}
   else if(bones[0]!=original||memcmp(info,&bind,sizeof bind))return false;
  }
  return skins[0]!=skins[1]&&!strcmp(scene::name(original),"Bip01 Spine")&&
         std::abs(bind.scale-1.f)<.001f&&scene::distance(bind.p,geometrySpec.bindPosition)<.01f;
 }

 bool attach(void* tree,const Rim& spec){
  scene::Transform bind{};if(!stockRim(tree,spec,bind))return false;
  basis=scene::find(tree,"FCStockWheelBasis");
  pivot=scene::find(basis,"FCSteeringWheel");
  driven=scene::find(pivot,"FCStockWheelSkin");
  if(!driven||scene::parent(basis)!=original||scene::parent(pivot)!=basis||scene::parent(driven)!=pivot)return false;
  scene::local(basis)=bind;
  scene::local(basis).p=bind.p+scene::rotate(bind,spec.center)*bind.scale;
  auto& inverse=scene::local(driven);
  for(int i=0;i<3;++i)for(int j=0;j<3;++j)inverse.r[i][j]=bind.r[j][i];
  inverse.scale=1.f/bind.scale;
  inverse.p=scene::inverseRotate(bind,bind.p)*(-inverse.scale)-spec.center;
  rim=&spec;
  root=tree;scene::retain(root);
  for(auto skin:skins){
   scene::retain(skin);
   scene::field<void**>(skin,0x14)[0]=driven;
  }
  scene::update(basis);
  return true;
 }

 void restore(){
  if(root){
   for(auto skin:skins){
    auto bones=scene::field<void**>(skin,0x14);
    if(bones&&bones[0]==driven)bones[0]=original;
    scene::release(skin);
   }
   if(scene::parent(basis))scene::moveParent(basis,nullptr);
   scene::release(root);
  }
  *this={};
 }
};
}
