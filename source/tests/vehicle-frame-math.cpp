#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cstdio>
#include <cstring>
#include "../vehicle-vr/mounted_frame.hpp"
#include "../vehicle-vr/contact_frame.hpp"
#include "../vehicle-vr/grip_attachment.hpp"
#include "../vehicle-vr/drive_input.hpp"
#include "../vehicle-vr/knee_steering.hpp"
#include "../empty_bottles.hpp"
using namespace mounted_frame;
int main(){
 Mat roll={{{1,0,0},{0,0,-1},{0,1,0}}};Mat yaw={{{0,-1,0},{1,0,0},{0,0,1}}};
 auto close=[](Vec a,Vec b){return scene::distance(a,b)<.0005f;};
 if(!close(apply(roll,{0,0,1}),{0,-1,0})||!close(apply(yaw,{1,0,0}),{0,1,0}))return 1;
 auto native=mul(yaw,roll);auto wanted=roll;auto correction=mul(wanted,transpose(native));
 // A world-aligned correction must retain the original stage-space direction.
 if(!close(apply(correction,apply(native,{1,2,3})),{1,-3,2}))return 2;
 for(int i=-50;i<=50;i++){Vec p{float(i)*.13f,float(i)*-.29f,float(i)*.06f};if(!close(apply(transpose(correction),apply(correction,p)),p))return 3;}
 auto roundTrip=mul(transpose(correction),mul(correction,native));
 for(int i=0;i<3;i++)for(int j=0;j<3;j++)if(std::abs(roundTrip.a[i][j]-native.a[i][j])>.00001f)return 4;
 // Removing the inverse must be detected by the fixture.
 if(close(apply(correction,apply(correction,{1,2,3})),{1,2,3}))return 5;
 // All physical boarding headings map to the same cabin forward. A later
 // physical head turn remains a relative turn, instead of recentering each frame.
 for(int degrees=-180;degrees<=180;degrees+=15){
  float angle=degrees*3.14159265f/180;Vec forward{-std::sin(angle),0,-std::cos(angle)};Mat heading;
  if(!headingFromForward(forward,heading))return 6;
  auto cabin=mul(roll,transpose(heading));
  if(!close(apply(cabin,forward),{0,1,0}))return 7;
  if(!close(apply(cabin,apply(heading,{1,0,0})),{1,0,0}))return 8;
 }
 Mat invalid;if(headingFromForward({0,1,0},invalid))return 9;
 for(int epoch=1;epoch<=3;epoch++){
  write({reinterpret_cast<void*>(epoch),correction,{1,2,3}});
  if(read().manager!=reinterpret_cast<void*>(epoch))return 10;
  nativeSetup=true;if(read().manager)return 11;
  nativeSetup=false;if(!read().manager)return 12;
  reset();if(read().manager)return 13;
 }
 // Contact solver history moves with a carrier, while authored controller
 // travel remains measurable. Exercise a wrapped partially occupied ring.
 scene::Transform old{},now{};old.scale=now.scale=1;auto id=identity();memcpy(old.r,id.a,sizeof old.r);memcpy(now.r,yaw.a,sizeof now.r);old.p={-72000,1800,8400};now.p={-71000,2300,8450};
 vehicle_contact::Delta delta;if(!vehicle_contact::makeDelta(old,now,delta))return 14;
 unsigned char state[0x81C]{};state[0]=state[0x1C]=1;scene::field<unsigned>(state,0x804)=3;scene::field<unsigned>(state,0x808)=1;
 Vec local{12,22,5},world=old.p+local,expected=now.p+apply(yaw,local);
 for(unsigned off:{4u,0x10u,0x30u})scene::field<Vec>(state,off)=world;
 scene::field<Vec>(state,0x20)={1,2,3};scene::field<Mat>(state,0x3C)=scene::field<Mat>(state,0x60)=id;
 for(unsigned slot:{0u,39u,38u}){scene::field<Vec>(state,0x84+slot*0x30)=world;scene::field<Mat>(state,0x90+slot*0x30)=id;}
 if(!vehicle_contact::validState(state))return 15;vehicle_contact::carryState(state,delta);
 for(unsigned off:{4u,0x10u,0x30u})if(!close(scene::field<Vec>(state,off),expected))return 16;
 if(!close(scene::field<Vec>(state,0x20),apply(yaw,{1,2,3})))return 17;
 for(unsigned slot:{0u,39u,38u})if(!close(scene::field<Vec>(state,0x84+slot*0x30),expected))return 18;
 if(!close(scene::field<Vec>(state,0x84+1*0x30),{}))return 19;
 if(!close(delta.point(world+Vec{1,0,0})-expected,apply(yaw,{1,0,0})))return 20;
 // Negative gate: no carry misclassifies vehicle travel as physical hand speed.
 if(scene::distance(world,expected)<100)return 21;
 scene::field<unsigned>(state,0x804)=41;if(vehicle_contact::validState(state))return 22;
 scene::field<unsigned>(state,0x804)=0;scene::field<unsigned>(state,0x808)=40;if(vehicle_contact::validState(state))return 23;
 now.scale=2;if(vehicle_contact::makeDelta(old,now,delta))return 24;
 // Non-unit carrier scales, rotation and translation preserve local grip.
 for(float scale:{.5f,1.f,2.f}){scene::Transform frame=old;frame.scale=scale;memcpy(frame.r,yaw.a,sizeof frame.r);scene::Transform pose=old;pose.p={-71995,1821,8412};memcpy(pose.r,roll.a,sizeof pose.r);
  auto localGrip=vehicle_grip::relative(frame,pose),back=vehicle_grip::world(frame,localGrip);
  if(!close(back.p,pose.p))return 25;for(int i=0;i<3;i++)for(int j=0;j<3;j++)if(std::abs(back.r[i][j]-pose.r[i][j])>.0001f)return 26;
  frame.p=frame.p+Vec{40,21,9};auto moved=vehicle_grip::world(frame,localGrip);if(!close(moved.p,pose.p+Vec{40,21,9}))return 27;
 }
 auto p=vehicle_drive::pedals(1,true,false,20);if(p.gas!=1||p.brake||p.changeGear)return 28;
 p=vehicle_drive::pedals(-1,true,false,20);if(p.gas||p.brake!=1||p.changeGear)return 29;
 p=vehicle_drive::pedals(-1,true,false,0);if(p.gas!=1||p.brake||!p.reverse||!p.changeGear)return 30;
 p=vehicle_drive::pedals(1,true,true,10);if(p.gas||p.brake!=1||p.changeGear)return 31;
 p=vehicle_drive::pedals(1,true,true,0);if(p.gas!=1||p.brake||p.reverse||!p.changeGear)return 32;
 p=vehicle_drive::pedals(.1f,true,false,10);if(p.gas||p.brake<=0)return 33;
 p=vehicle_drive::pedals(1,false,false,10);if(p.gas||p.brake!=1)return 34;
 auto kneeRot=knee_steering::align({0,30,0},{0,26,15});auto turned=apply(kneeRot,{0,30,0});if(std::abs(scene::distance(turned,{})-30)>.001f||turned.z<14)return 35;
 if(empty_bottles::baseForModel("Clutter/LiquorBottles/WhiskeyBottle01.NIF")!=0x5B6CE||empty_bottles::baseForModel("clutter/food/Iguana.nif"))return 36;
 unsigned char manager[0x1A80]{};
 scene::field<unsigned>(manager,0x19F8)=0x13572468;scene::field<unsigned>(manager,0x1A5C)=0x89ABCDEF;
 for(int h=0;h<2;h++){scene::field<Vec>(manager,0x19FC+h*12)={1.f+h,2,3};scene::field<Mat>(manager,0x1A14+h*36)=id;}
 vehicle_contact::carryManager(manager,delta);
 if(scene::field<unsigned>(manager,0x19F8)!=0x13572468||scene::field<unsigned>(manager,0x1A5C)!=0x89ABCDEF)return 37;
 for(int h=0;h<2;h++)if(!close(scene::field<Vec>(manager,0x19FC+h*12),delta.vector({1.f+h,2,3})))return 38;
 printf("PASS: knee rotation preserves lengths; bottle allowlist;  grip attachment transforms/scales; signed driving/brake-before-reverse/neutral/tracking loss;  frame axes/round trips/boarding/reset; moving-contact translation+rotation, wrapped-ring validity, real hand motion, scale discontinuity and negative stale-history fixture\n");return 0;
}
