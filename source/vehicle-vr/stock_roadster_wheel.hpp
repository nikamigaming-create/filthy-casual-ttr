#pragma once

// Retail NiGeometry/NiSkinInstance layout shared with world_shadow_scene.
// The two 8b Roadster rim shapes are each fully weighted to the same spine.
namespace stock_roadster_wheel {
inline constexpr scene::Vec center{-19.65369f,-23.72104f,82.347f};
inline constexpr scene::Vec up{0.f,.3420203f,.9396926f};
inline constexpr float radius=9.97413f;

struct Rig {
 void* root{};
 void* basis{};
 void* pivot{};
 void* driven{};
 void* original{};
 void* skins[2]{};

 bool stockRim(void* tree,scene::Transform& bind){
  const char* names[]={"SteeringWheelA","SteeringWheelB"};
  const unsigned short vertexCounts[]={190,52};
  for(int i=0;i<2;++i){
   auto shape=scene::find(tree,names[i]);if(!shape)return false;
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
         std::abs(bind.scale-1.f)<.001f&&scene::distance(bind.p,{0.f,93.77938f,-38.42404f})<.01f;
 }

 bool attach(void* tree){
  scene::Transform bind{};if(!stockRim(tree,bind))return false;
  basis=scene::find(tree,"FCStockWheelBasis");
  pivot=scene::find(basis,"FCSteeringWheel");
  driven=scene::find(pivot,"FCStockWheelSkin");
  if(!driven||scene::parent(basis)!=original||scene::parent(pivot)!=basis||scene::parent(driven)!=pivot)return false;
  scene::local(basis)=bind;
  scene::local(basis).p=bind.p+scene::rotate(bind,center)*bind.scale;
  auto& inverse=scene::local(driven);
  for(int i=0;i<3;++i)for(int j=0;j<3;++j)inverse.r[i][j]=bind.r[j][i];
  inverse.scale=1.f/bind.scale;
  inverse.p=scene::inverseRotate(bind,bind.p)*(-inverse.scale)-center;
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
