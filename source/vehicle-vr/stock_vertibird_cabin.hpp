#pragma once

namespace stock_vertibird_cabin {
struct Rig {
 void* root{};
 void* carrier{};
 void* seat{};
 void* originalSeatParent{};
 scene::Transform originalSeat{};
 void* body[2]{};
 unsigned bodyFlags[2]{};

 bool mount(void* tree){
  auto frame=scene::find(tree,"FCCockpitCarrier");
  auto cabin=scene::find(frame,"FCStockCabin");
  auto anchor=scene::find(frame,"FCStockPilotAnchor");
  auto eye=scene::find(frame,"FCPilotEye");
  auto nativeSeat=scene::find(tree,"PlayerPos");
  auto hull=scene::find(tree,"BodyMain");
  auto tail=scene::find(tree,"BodyTail");
  if(!cabin||!anchor||!eye||!nativeSeat||!hull||!tail)return false;
  if(!scene::find(cabin,"VertibirdHull01:0")||!scene::find(cabin,"VertibirdHull01:1")||
     !scene::find(cabin,"VertibirdConsole01:0"))return false;
  if(!scene::find(frame,"FCFlightGripR")||!scene::find(frame,"FCFlightGripL"))return false;
  auto skin=scene::field<void*>(hull,0xBC);if(!skin)return false;
  auto skinData=scene::field<void*>(skin,8);
  auto bones=scene::field<void**>(skin,0x14);
  auto bind=skinData?scene::field<scene::Transform*>(skinData,0x40):nullptr;
  if(!bind||!bones||scene::field<unsigned>(skinData,0x44)!=1||
     !bones[0]||scene::parent(frame)!=bones[0]||!std::isfinite(bind->scale)||std::abs(bind->scale-1.f)>.01f)return false;

  root=tree;scene::retain(root);carrier=frame;
  seat=nativeSeat;originalSeatParent=scene::parent(seat);originalSeat=scene::local(seat);
  body[0]=hull;body[1]=tail;
  for(int i=0;i<2;++i)bodyFlags[i]=scene::field<unsigned>(body[i],0x30)&1u;
  scene::local(frame)=*bind;
  scene::local(cabin)={{{-1,0,0},{0,-1,0},{0,0,1}},{0,90,582},.75f};
  scene::local(anchor).p={26,268,489};
  scene::local(eye).p={26,140,566};
  scene::moveParent(seat,anchor);
  scene::local(seat)={{{1,0,0},{0,1,0},{0,0,1}},{0,-128,0},1};
  if(auto windowCaps=scene::find(cabin,"VertibirdHull01:2"))scene::field<unsigned>(windowCaps,0x30)|=1u;
  for(auto node:body)scene::field<unsigned>(node,0x30)|=1u;
  scene::update(frame);
  return true;
 }

 void restore(){
  if(root){
   if(seat&&scene::parent(seat)==scene::find(carrier,"FCStockPilotAnchor")){
    scene::moveParent(seat,originalSeatParent);scene::local(seat)=originalSeat;scene::update(seat);
   }
   for(int i=0;i<2;++i)scene::field<unsigned>(body[i],0x30)=(scene::field<unsigned>(body[i],0x30)&~1u)|bodyFlags[i];
   if(scene::parent(carrier))scene::moveParent(carrier,nullptr);
   scene::release(root);
  }
  *this={};
 }
};
}
