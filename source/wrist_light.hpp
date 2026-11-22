#pragma once
#include "scene_adapter.hpp"
#include "build_adapter.hpp"
#include "vehicle-vr/road_contacts.hpp"
#include "lamp_clearance.hpp"
#include "wrist_light_config.hpp"
#include "lamp_light_adapter.hpp"
#include "lamp_falloff.hpp"
#include "projected_render_adapter.hpp"
// Wrist-directed projected light with native lighting fallback.
// Original lamp/cord attachment and native toggle; no persistent world reference.
namespace wrist_light {
inline Config config;
inline bool baselineComparison=false; // Private simulator A/B only; never persisted.
inline void* player=nullptr;
inline bool gameplay=false,installed=false,temporalCopy=false;
inline unsigned calls=0,draws=0,depthFormat=0,fail=0;
inline unsigned fillHits=0,lightingSamples=0;inline double lightingMicros=0,lightingMaxMicros=0;
inline scene::Vec lastOrigin{},lastDirection{},lastHit{};
inline bool hit=false;
inline void* changed=nullptr;
inline scene::WorldPin beamPin;
inline void* airBeam[9]{};
inline void* lamp=nullptr;inline void* gloveBand=nullptr;inline void* mount=nullptr;inline ULONGLONG deployAt=0;
inline void* cable[64]{};inline void* plug=nullptr;inline scene::Vec cordStart{},cordEnd{};
inline scene::Vec cableAcross{};inline bool cableFrameValid=false;
inline void* pipAnchor=nullptr;inline ULONGLONG pipRetry=0;inline bool fixedSocket=false;
inline scene::Vec socketLocal{},socketNormal{};
inline void* stockBeamShape=nullptr;
inline void* stockBeamParent=nullptr;
inline void restoreStockBeam(){
 if(!stockBeamShape)return;
 if(!scene::parent(stockBeamShape)){
  scene::moveParent(stockBeamShape,stockBeamParent);
  scene::update(stockBeamShape);
 }
 scene::release(stockBeamShape);stockBeamShape=nullptr;
 scene::release(stockBeamParent);stockBeamParent=nullptr;
}
inline void suppressStockBeam(void* root){
 if(stockBeamShape){
  // A native mesh refresh can reattach the retained beam during the same toggle.
  if(scene::parent(stockBeamShape)==stockBeamParent)scene::moveParent(stockBeamShape,nullptr);
  return;
 }
 auto effect=scene::find(root,"PipboyLightEffect");
 auto shape=scene::find(effect,"PipboyLightEffect:0");
 if(!shape||scene::parent(shape)!=effect)return;
 // Keep the stock effect node and light intact. Only its visible beam mesh
 // is held out of the scene while the world-space projector is enabled.
 scene::retain(effect);scene::retain(shape);
 scene::moveParent(shape,nullptr);
 if(scene::parent(shape)){
  scene::release(shape);scene::release(effect);return;
 }
 stockBeamParent=effect;stockBeamShape=shape;
}
inline void clearSocket(){restoreStockBeam();scene::release(pipAnchor);pipAnchor=nullptr;pipRetry=0;fixedSocket=false;cableFrameValid=false;lamp_clearance::valid=false;}
inline void* pipSocket(void* rig){
 if(pipAnchor)return pipAnchor;
 auto now=GetTickCount64();if(now<pipRetry)return nullptr;pipRetry=now+1000;
 // TTR can rename/reparent the imported root; the original body mesh remains
 // identifiable. Use its direct parent, the NIF's actual device frame.
 auto search=rig;while(search&&scene::parent(search))search=scene::parent(search);
 auto body=scene::find(search,"PipBoyArm:0");
 bool female=false;
 if(!body){body=scene::find(search,"PipBoyArmFemale:0");female=body!=nullptr;}
 auto p=body?scene::parent(body):scene::find(search,"PipBoyArm");
 if(p){
  pipAnchor=p;scene::retain(pipAnchor);lamp_clearance::build(p);
  // Seat the collar into the lower case wall on the FILTHY side. The
  // stock male and female shells put that wall at slightly different depths.
  socketLocal={12.52f,-2.38f,3.11f};
  if(auto data=scene::find(p,"PipBoyButton03"))
   socketLocal=scene::local(data).p+scene::Vec{.65f,female?-.64f:-.71f,-1.20f};
  socketNormal={female?.05f:-.08f,-1.f,.14f};
  socketNormal=socketNormal*(1.f/scene::distance(socketNormal,{}));
 }
 return p;
}
inline bool bindBeam(void* root){
 if(!root||!beamPin.attach(root))return false;
 lamp=scene::find(root,"FCLampGear");mount=scene::find(root,"FCLampMount");
 gloveBand=scene::find(root,"FCGloveBand");plug=scene::find(root,"FCLampPlug");
 for(int i=0;i<64;i++){char name[32];sprintf_s(name,"FCLampCable%02d",i);cable[i]=scene::find(root,name);}
 for(int i=0;i<9;i++){char name[32];sprintf_s(name,"FCBeam%d",(i+1)*10);airBeam[i]=scene::find(root,name);}
 return true;
}
inline void cableSegment(void* node,scene::Vec a,scene::Vec b){
 if(!node)return;auto delta=b-a;float len=scene::distance(delta,{});if(len<.001f)return;
 auto z=delta*(1/len),x=scene::cross(z,{0,0,1});float xl=scene::distance(x,{});
 if(xl<.001f)x={1,0,0};else x=x*(1/xl);auto y=scene::cross(z,x);
 // Stretch along the wire only; its physical diameter remains constant.
 scene::Transform t{{{x.x,y.x,delta.x},{x.y,y.y,delta.y},{x.z,y.z,delta.z}},a,1};scene::setWorld(node,t);
}
inline void hideBeam(){projected_depth_adapter::requested=false;projected_lighting::publish(false,nullptr,nullptr,0,0,nullptr,0);lamp_lights::disable();if(beamPin.node)scene::field<unsigned>(beamPin.node,0x30)|=1;deployAt=0;}
inline scene::Transform originalLocal{};
inline float originalFade=0,originalRadius=0;
inline scene::Vec originalColour{};
inline void* nativeLight(){
 if(!player)return nullptr;
 auto item=reinterpret_cast<unsigned char*(__thiscall*)(void*)>(0x93CCD0)(nullptr);if(!item)return nullptr;
 auto target=static_cast<unsigned char*>(player)+0x94;auto vt=scene::field<void**>(target,0);
 struct Entry{unsigned char* effect;Entry* next;};
 auto first=reinterpret_cast<Entry*(__thiscall*)(void*)>(vt[2])(target);
 for(unsigned n=0;first&&n<128;first=first->next,++n){auto e=first->effect;
  if(e&&scene::field<unsigned>(e,0)==0x107A2CC&&scene::field<void*>(e,8)==item+0x18)return scene::field<void*>(e,0x48);
 }return nullptr;
}
inline void restore(){
 restoreStockBeam();
 auto light=nativeLight();if(light&&light==changed){
  scene::local(light)=originalLocal;scene::field<float>(light,0xC4)=originalFade;
  scene::field<float>(light,0xE0)=originalRadius;scene::field<scene::Vec>(light,0xD4)=originalColour;
  scene::update(light);
 }changed=nullptr;hit=false;hideBeam();
}
inline void publish(bool completed){
 ++calls;if(!completed||!installed)return;++lamp_shadow_renderer::generation;
 temporalCopy=build_adapter::temporalEyeCopyActive();
 // Project against the matched world colour and depth before the eye copy.
 projected_render_adapter::install(gameplay&&config.enabled&&config.projected);
 auto light=nativeLight();if(!light){restoreStockBeam();changed=nullptr;hit=false;hideBeam();return;}
 if(!config.enabled||!gameplay){restore();return;}
 auto root=scene::root(player);
 auto pip=pipSocket(root);fixedSocket=pip!=nullptr;
 if(config.projected)suppressStockBeam(pip?pip:root);else restoreStockBeam();
 auto hand=scene::find(root,"Bip01 L Hand");
 auto index=scene::find(hand,"Bip01 L Finger1"),pinky=scene::find(hand,"Bip01 L Finger4");
 auto forearm=scene::find(root,"Bip01 L Forearm");
 if(!hand||!index||!pinky||!forearm){if(config.projected)hideBeam();else restore();return;}auto h=scene::world(hand),arm=scene::world(forearm);
 // Proximal knuckle positions follow the wrist, not forearm IK or fingertip curl.
 auto a=scene::world(index).p-h.p,b=scene::world(pinky).p-h.p;
 auto direction=(a+b)*.5f,dorsal=scene::cross(a,b);
 float length=scene::distance(direction,{}),dl=scene::distance(dorsal,{});
 if(!(length>=.01f && dl>=.01f)){if(config.projected)hideBeam();else restore();return;}
 direction=direction*(1/length);dorsal=dorsal*(1/dl);
 if(!deployAt)deployAt=GetTickCount64();
 float extension=std::clamp(float(GetTickCount64()-deployAt)/180.f,0.f,1.f);extension=extension*extension*(3-2*extension);
 // Power socket stays on the Pip-Boy. The flexible lead follows the lamp.
 auto armForward=scene::rotate(arm,{1,0,0}),armUp=scene::rotate(arm,{0,-1,0});
 auto mountPoint=h.p+armForward*config.forward+armUp*config.above;
 // Lamp keeps its wrist-guided position; the device anchor controls only the plug.
 auto pivot=mountPoint+armUp*(.35f+.65f*extension);
 lastDirection=direction;lastOrigin=pivot+direction*1.f;
 // Check the entire barrel, not just its emitter. At extreme wrist angles,
 // slide the wired lamp clear of the glove/cuff without changing its aim.
 auto knuckles=h.p+direction*7.f;
 if(config.gloveMount){
  // Left palm is authored +Y; index-to-pinky cross points to the dorsal -Y.
  // Knuckle mount: use the completed hand frame, never a world-space push.
  // Keep beam aim wrist-guided; the housing stays in that hand frame.
  lastOrigin=h.p+direction*4.f+dorsal*4.1f;
 }
 for(int pass=0;!config.gloveMount&&pass<10;pass++){
  float moved=0;
  for(float along:{0.f,1.2f,2.45f}){
   auto sample=lastOrigin-direction*along;
   auto push=lamp_clearance::casePush(pip,sample,1.05f);lastOrigin=lastOrigin+push;moved+=scene::distance(push,{});
   sample=lastOrigin-direction*along;
   push=lamp_clearance::handPush(sample,h.p,knuckles,dorsal,1.05f);lastOrigin=lastOrigin+push;moved+=scene::distance(push,{});
  }if(moved<.01f)break;
 }
 projected_depth_adapter::requested=projected_lighting::enabled;
 // A requested projector never turns into a point light while the depth
 // surface is being acquired, the shader is warming up or an eye is skipped.
 const bool projected=config.projected;
 LARGE_INTEGER costStart,costEnd,costFrequency;QueryPerformanceCounter(&costStart);
 const float reach=baselineComparison?1400.f:config.range;
 auto end=lastOrigin+direction*reach;scene::Vec normal{};Form* owner=nullptr;
 hit=road_contacts::ray(lastOrigin+direction*8,end,normal,owner,static_cast<Form*>(player),static_cast<Form*>(player));
 if(changed!=light){changed=light;originalLocal=scene::local(light);originalFade=scene::field<float>(light,0xC4);originalRadius=scene::field<float>(light,0xE0);originalColour=scene::field<scene::Vec>(light,0xD4);}
 lastHit=end;
 auto target=scene::world(light);float distance=scene::distance(lastOrigin,end);
 projected_lighting::publish(projected_lighting::enabled,&lastOrigin.x,&direction.x,config.range,config.angle,config.rgb,config.brightness);
 float radius=std::clamp(distance*std::tan(config.angle*.5f*.0174532925f),baselineComparison?20.f:30.f,baselineComparison?350.f:850.f);
 // Offset toward the viewer-facing side of the hit so the surface is lit.
 target.p=end+normal*std::min(radius*.35f,40.f);
 scene::field<float>(light,0xC4)=hit&&!projected?config.brightness*(baselineComparison?1.f:lamp_falloff::fade(distance,reach)):0;
 scene::field<float>(light,0xE0)=radius;
 scene::field<scene::Vec>(light,0xD4)={config.rgb[0],config.rgb[1],config.rgb[2]};
 scene::field<bool>(light,0x9E)=false;
 scene::setWorld(light,target);
 // Refresh the renderer's cached light centre/radius after wrist relocation.
 if(auto shadow=*reinterpret_cast<void**>(0x11F91C8)){
  struct Entry{Entry* next;Entry* prev;void* data;};
  auto entry=scene::field<Entry*>(shadow,0xB4);
  for(unsigned i=0;entry&&i<1024;entry=entry->next,++i)if(entry->data&&scene::field<void*>(entry->data,0xF8)==light){reinterpret_cast<void(__thiscall*)(void*,void*)>(0xB5D300)(shadow,entry->data);break;}
 }
 // Three stable perimeter samples fill side walls/ground even when the centre
 // misses. Fixed pool, four rays total per publication; no per-pixel history.
 lamp_lights::activeCount=0;fillHits=0;
 auto fillAcross=scene::cross(direction,dorsal);float acrossLen=scene::distance(fillAcross,{});
 if(!projected&&!baselineComparison&&acrossLen>.001f&&lamp_lights::verify()){
  fillAcross=fillAcross*(1/acrossLen);auto up=scene::cross(fillAcross,direction);
  float spread=std::tan(config.angle*.5f*.0174532925f)*.90f;
  for(int i=0;i<3;i++){
   float theta=(i*120.f+90.f)*.0174532925f;
   auto ray=direction+(fillAcross*std::cos(theta)+up*std::sin(theta))*spread;
   ray=ray*(1/scene::distance(ray,{}));auto point=lastOrigin+ray*config.range;scene::Vec n{};Form* object=nullptr;
   bool found=road_contacts::ray(lastOrigin+ray*8,point,n,object,static_cast<Form*>(player),static_cast<Form*>(player));
   float d=scene::distance(lastOrigin,point),rad=std::clamp(d*std::tan(config.angle*.5f*.0174532925f)*1.05f,40.f,750.f);
   float power=found?config.brightness*.65f*lamp_falloff::fade(d,config.range):0;
   if(found)++fillHits;
   lamp_lights::apply(i,point+n*std::min(rad*.28f,40.f),rad,power,config.rgb);
  }
 }
 if(projected||baselineComparison||acrossLen<=.001f||!lamp_lights::supported)lamp_lights::disable();
 QueryPerformanceCounter(&costEnd);QueryPerformanceFrequency(&costFrequency);
 double us=double(costEnd.QuadPart-costStart.QuadPart)*1e6/double(costFrequency.QuadPart);
 lightingMicros+=us;lightingMaxMicros=std::max(lightingMaxMicros,us);++lightingSamples;
 if(beamPin.node){
  scene::field<unsigned>(beamPin.node,0x30)&=~1u;
  auto y=direction;auto x=scene::cross(y,dorsal);float xl=scene::distance(x,{});x=x*(1/xl);auto z=scene::cross(x,y);
  scene::Transform frame{{{x.x,y.x,z.x},{x.y,y.y,z.y},{x.z,y.z,z.z}},lastOrigin,projected?1.f:std::max(.01f,std::min(distance,std::min(reach,840.f)))};
  scene::setWorld(beamPin.node,frame);
  const int beamAngle=std::clamp(int(std::round(config.angle/10.f))-1,0,8);
  for(int i=0;i<9;i++)if(airBeam[i]){
   auto& flags=scene::field<unsigned>(airBeam[i],0x30);
   // Older MO2 asset stacks still ship crossed transparent sheets. The world
   // projector owns the beam in this mode; keep only the lamp and its lead.
   if(!projected && i==beamAngle)flags&=~1u;else flags|=1u;
  }
  if(lamp){auto hardware=frame;hardware.scale=1;scene::setWorld(lamp,hardware);
   if(gloveBand)scene::field<unsigned>(gloveBand,0x30)|=1u; // Hide legacy rings on older installed meshes.
  }
  auto side=scene::cross(armForward,armUp);auto port=mountPoint-armForward*3+side*1.1f;
  auto portForward=armForward,portUp=armUp;
  if(pip){
   const auto p=scene::world(pip);
   port=p.p+scene::rotate(p,socketLocal)*p.scale;
   portUp=scene::rotate(p,socketNormal);
   auto across=scene::cross({1,0,0},socketNormal);
   side=scene::rotate(p,across*(1.f/scene::distance(across,{})));
   portForward=scene::cross(portUp,side);
  }
  if(mount){scene::Transform socket{{{side.x,portForward.x,portUp.x},{side.y,portForward.y,portUp.y},{side.z,portForward.z,portUp.z}},port,1};scene::setWorld(mount,socket);}
  auto rear=lastOrigin-direction*2.45f;
  cableSegment(plug,rear,rear-direction*.55f);
  cordStart=port+portUp*.65f;cordEnd=rear-direction*.55f;
  // Lead out of the downward-facing socket before bending back toward the hand.
  const auto bendStart=cordStart+portUp*.42f;
  cableSegment(cable[0],cordStart,bendStart);
  auto delta=cordEnd-bendStart;float span=scene::distance(delta,{});
  if(span>.01f){
   auto dot=[](scene::Vec a,scene::Vec b){return a.x*b.x+a.y*b.y+a.z*b.z;};
   auto axis=delta*(1/span),reference=cableFrameValid?cableAcross:side;
   auto across=reference-axis*dot(reference,axis);float al=scene::distance(across,{});
   if(al<.01f){reference=std::abs(dot(axis,portUp))<.9f?portUp:portForward;across=reference-axis*dot(reference,axis);al=scene::distance(across,{});}
   if(al>.01f){across=across*(1/al);cableAcross=across;cableFrameValid=true;
    // Transport the coil frame instead of flipping between world-up axes at
    // extreme twists. Keep slack when compressed, and lift the middle clear
    // of the case when the lamp is aimed back across it.
    float extreme=std::clamp((.45f-dot(direction,portForward))/.8f,0.f,1.f);
    extreme=extreme*extreme*(3-2*extreme);
    auto bow=armUp*.8f+portUp*(extreme*2.0f+std::max(0.f,1.8f-span))+portForward*(extreme*1.5f);
    const auto exitControl=bendStart+portUp*std::clamp(span*.32f,.7f,2.4f);
    const auto entryControl=cordEnd-direction*std::clamp(span*.21f,.35f,1.5f);
    constexpr int lastIndex=63;
    constexpr float pi=3.14159265f;
    scene::Vec centers[lastIndex+1]{};
    centers[0]=bendStart;centers[lastIndex]=cordEnd;
    const auto clearCenter=[&](scene::Vec point,float t){
     const float edge=std::min(t,1.f-t);
     const float clearanceRadius=.10f+.38f*std::clamp(edge*6.f,0.f,1.f);
     for(int pass=0;pass<2;++pass){
      point=point+lamp_clearance::handPush(point,h.p,knuckles,dorsal,clearanceRadius);
      point=point+lamp_clearance::casePush(pip,point,clearanceRadius,true);
     }
     return point;
    };
    // Clear the centerline before drawing the helix. Nearby samples share
    // the same route and can bend smoothly around the underside of the case.
    for(int i=1;i<lastIndex;++i){
     const float t=float(i)/lastIndex,u=1-t,envelope=std::sin(t*pi);
     const auto center=bendStart*(u*u*u)+exitControl*(3*u*u*t)+entryControl*(3*u*t*t)+cordEnd*(t*t*t);
     centers[i]=clearCenter(center+bow*(envelope*envelope),t);
    }
    for(int pass=0;pass<2;++pass){
     auto previous=centers[0];
     for(int i=1;i<lastIndex;++i){
      auto current=centers[i];
      centers[i]=clearCenter((previous+current*2.f+centers[i+1])*.25f,float(i)/lastIndex);
      previous=current;
     }
    }
    float distance[lastIndex+1]{};
    for(int i=1;i<=lastIndex;++i)distance[i]=distance[i-1]+scene::distance(centers[i],centers[i-1]);
    const float total=std::max(distance[lastIndex],.001f);
    auto last=bendStart,normal=across;
    int interval=0;
    for(int i=1;i<lastIndex;++i){
     const float t=float(i)/lastIndex,target=t*total;
     while(interval+1<lastIndex&&distance[interval+1]<target)++interval;
     const float width=std::max(distance[interval+1]-distance[interval],.001f);
     const float fraction=std::clamp((target-distance[interval])/width,0.f,1.f);
     const auto center=centers[interval]*(1.f-fraction)+centers[interval+1]*fraction;
     auto tangent=centers[std::min(interval+2,lastIndex)]-centers[std::max(interval-1,0)];
     const float tangentLength=scene::distance(tangent,{});
     tangent=tangentLength>.001f?tangent*(1/tangentLength):axis;
     auto projectedNormal=normal-tangent*dot(normal,tangent);
     const float normalLength=scene::distance(projectedNormal,{});
     if(normalLength>.001f)normal=projectedNormal*(1/normalLength);
     const auto around=scene::cross(tangent,normal);
     const float phase=t*10*pi;
     const float fade=std::clamp(std::min(t,1.f-t)*5.f,0.f,1.f);
     const float coilRadius=.30f*fade*fade*(3.f-2.f*fade);
     const auto next=center+(normal*std::cos(phase)+around*std::sin(phase))*coilRadius;
     cableSegment(cable[i],last,next);
     last=next;
    }
    cableSegment(cable[lastIndex],last,cordEnd);
   }
  }
 }
 ++draws;
}
inline bool install(){installed=build_adapter::heldModule()!=nullptr;return installed;}
}
