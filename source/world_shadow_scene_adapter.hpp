#pragma once
#include "scene_adapter.hpp"
#include <d3d9.h>
#include "world_visit_set.hpp"
#include "world_shadow_head_adapter.hpp"
#include "world_shadow_grass_adapter.hpp"
#include "world_shadow_gpu_adapter.hpp"
#include <unordered_map>
#include <memory>
// FNV 1.4.0.525 only, after the exact TTR build guard. Layout evidence:
// JIP netimmerse.h (geometry, texture, alpha); native GameNi.h (skin weights).
// Snapshot at the guarded world-end on the rendering thread. No node retained.
namespace world_shadow_scene {
using scene::Vec;using scene::field;
struct Vertex {Vec p;float u=0,v=0,a=1,instance=0;};
struct Mesh {
 std::vector<Vertex> vertices;std::vector<unsigned short> indices;void* retained=nullptr;
 std::array<unsigned,228> grassEnds{};
 std::array<unsigned,228> grassVertexEnds{};
 IDirect3DVertexBuffer9* vb=nullptr;IDirect3DIndexBuffer9* ib=nullptr;bool uploaded=false;
 ~Mesh(){if(vb)vb->Release();if(ib)ib->Release();if(retained)scene::release(retained);}
 bool upload(IDirect3DDevice9* d){if(uploaded)return true;
  if(!vb&&FAILED(d->CreateVertexBuffer(UINT(vertices.size()*sizeof(Vertex)),D3DUSAGE_WRITEONLY,0,D3DPOOL_DEFAULT,&vb,nullptr)))return false;
  if(!ib&&FAILED(d->CreateIndexBuffer(UINT(indices.size()*2),D3DUSAGE_WRITEONLY,D3DFMT_INDEX16,D3DPOOL_DEFAULT,&ib,nullptr)))return false;
  void* p=nullptr;if(FAILED(vb->Lock(0,0,&p,0)))return false;memcpy(p,vertices.data(),vertices.size()*sizeof(Vertex));vb->Unlock();
  if(FAILED(ib->Lock(0,0,&p,0)))return false;memcpy(p,indices.data(),indices.size()*2);ib->Unlock();uploaded=true;return true;
 }
};
inline std::unordered_map<void*,std::shared_ptr<Mesh>> cache;
inline size_t cacheBytes=0;
inline unsigned overflowFailures=0,emptyFailures=0,readFailures=0;
inline unsigned lastOverflowReason=0,lastOverflowTriangles=0,lastOverflowVertices=0,lastOverflowBatches=0,lastOverflowNodes=0;
struct Batch {std::shared_ptr<Mesh> mesh;scene::Transform pose{};bool skinned=false,vertexAlpha=false;IDirect3DBaseTexture9* alpha=nullptr;float threshold=0,opacity=1;bool translucent=false;Vec centre{};float radius=0;int grass=-1,gpu=-1;unsigned indexCount=0;};
struct RoomLight {Vec position{},colour{};float radius=0;uintptr_t identity=0;};
inline uintptr_t previousRoomLights[2]{};
struct Snapshot {
 std::vector<Batch> batches;WorldVisitSet visited;world_shadow_head::View head;
 std::vector<world_shadow_grass::InstanceData> grasses;
 std::vector<world_shadow_gpu::Geometry> gpuMeshes;unsigned gpuTriangles=0,gpuVertices=0,gpuSkipped=0;
 unsigned triangles=0,vertices=0,nodes=0,skinned=0,alpha=0,skipped=0,grassSkipped=0,lod=0;bool overflow=false;
 Vec origin{},sunDirection{},sunColour{};float casterReach=0;bool sun=false;RoomLight room[2]{};unsigned roomCount=0;
 void clear(){for(auto& b:batches)if(b.alpha)b.alpha->Release();batches.clear();grasses.clear();gpuMeshes.clear();gpuTriangles=gpuVertices=gpuSkipped=0;visited.clear();head.count=0;head.ready=false;head.root=head.face=nullptr;triangles=vertices=nodes=skinned=alpha=skipped=0;grassSkipped=lod=0;overflow=false;sun=false;roomCount=0;room[0]={};room[1]={};}
 ~Snapshot(){clear();}
};
inline bool finite(Vec v){return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);}
inline Vec transformed(const scene::Transform& t,Vec p){return t.p+scene::rotate(t,p)*t.scale;}
inline bool deform(void* shape,std::vector<Vertex>& out,const world_shadow_head::View& head){
 auto skin=field<void*>(shape,0xBC);if(!skin)return false;
 auto data=field<void*>(skin,8);auto bones=field<void**>(skin,0x14);
 if(!data||!bones)return false;
 auto count=field<unsigned>(data,0x44);auto info=field<unsigned char*>(data,0x40);
 if(!count||count>256||!info)return false;
 std::vector<Vec> sums(out.size());std::vector<float> weights(out.size());
 for(unsigned b=0;b<count;++b){if(!bones[b])return false;auto bone=info+b*0x4C;
  auto bind=field<scene::Transform>(bone,0);auto pose=head.world(bones[b]);
  auto n=field<unsigned short>(bone,0x48);auto w=field<unsigned char*>(bone,0x44);
  if(n&&!w)return false;
  for(unsigned k=0;k<n;++k){auto id=field<unsigned short>(w+k*8,0);float weight=field<float>(w+k*8,4);
   if(id>=out.size()||!std::isfinite(weight)||weight<0||weight>1.001f)return false;
   sums[id]=sums[id]+transformed(pose,transformed(bind,out[id].p))*weight;weights[id]+=weight;
  }
 }
 for(size_t i=0;i<out.size();++i){if(weights[i]<.95f||weights[i]>1.05f||!finite(sums[i]))return false;out[i].p=sums[i]*(1/weights[i]);}
 return true;
}
// The flashlight admits ordinary source-over materials to the same mask pass
// as cutouts. Decals, billboards, refraction and additive effects do not
// describe a solid surface. Preserve their native light transmission.
inline bool surfaceOpacity(void* prop, unsigned blendFlags) {
 const unsigned flags1=field<unsigned>(prop,0x20),flags2=field<unsigned>(prop,0x24);
 constexpr unsigned effectFlags=0x8000|0x10000|0x400000|0x4000000|0x8000000;
 constexpr unsigned effectFlags2=0x10|0x2000|0x10000|0x200000;
 if((flags1&effectFlags)||(flags2&effectFlags2))return false;
 const unsigned source=(blendFlags>>1)&15,destination=(blendFlags>>5)&15;
 return destination==7&&(source==6||(source==0&&(flags2&0x80000)));
}
inline void geometry(void* node,Snapshot& s){
 auto data=field<void*>(node,0xB8);auto prop=field<void*>(node,0xA8);if(!data||!prop)return;
 unsigned type=field<unsigned>(prop,0x1C);
 // Local meshes and actors also live under ObjectLODRoot. Exclude only geometry
 // tagged by the engine as distant landscape, buildings, or distant object LOD.
 if(s.casterReach>0&&(type==2||(type==8&&(field<unsigned>(prop,0x24)&6))))return;
 // Sky, particles, water and unverified SpeedTree formats do
 // not become opaque card casters. Ordinary alpha-tested foliage is supported.
 if(type!=1&&type!=2&&type!=4&&type!=8&&type!=9&&type!=0xA&&type!=0xC){++s.skipped;return;}
 auto bound=field<void*>(node,0x20);if(!bound)return;
 float radius=field<float>(bound,0xC);Vec centre=field<Vec>(bound,0);
 if(!finite(centre)||!std::isfinite(radius)||(s.casterReach>0&&scene::distance(centre,s.origin)>s.casterReach+std::abs(radius)))return;
 auto n=field<unsigned short>(data,8);auto positions=field<Vec*>(data,0x20);
 if(!n)return;
 auto alpha=field<void*>(node,0x9C);unsigned af=alpha?field<unsigned short>(alpha,0x18):0;
 const bool blendOnly=(af&1)&&!(af&512);
 float coverageOpacity=1;
 if(blendOnly){
  if(s.casterReach<=0||type!=8||!surfaceOpacity(prop,af))return;
  auto material=field<void*>(node,0xA4);
  const float opacity=field<float>(prop,0x28)*(material?field<float>(material,0x3C):1.f);
  if(!std::isfinite(opacity)||opacity<=0.f)return;
  coverageOpacity=std::min(opacity,1.f);
 }
 if(!blendOnly&&field<float>(prop,0x28)<.2f)return;
 Batch b;b.translucent=blendOnly;b.opacity=coverageOpacity;b.centre=centre-s.origin;b.radius=std::abs(radius);b.skinned=field<void*>(node,0xBC)!=nullptr;auto uv=field<float*>(data,0x2C);auto colours=field<float*>(data,0x28);
 // Many local cutouts retain CPU positions but keep their UVs only in the
 // native vertex buffer. Use its declaration and diffuse texture together.
 const bool gpuCutout=type==8&&!b.skinned&&((af&512)||blendOnly)&&(!positions||!uv);
 if(!positions||gpuCutout){
  // Native distant opaque meshes have already discarded their CPU copies.
  // Reuse the verified GPU draw packet, including its declaration and stride.
  if(!gpuCutout&&(type!=8||b.skinned||(af&512)||(!s.casterReach&&!(field<unsigned>(prop,0x24)&6)))){++s.gpuSkipped;return;}
  world_shadow_gpu::Geometry gpu;
  if(!world_shadow_gpu::read(data,gpu,gpuCutout,gpuCutout&&(field<unsigned>(prop,0x20)&8))){++s.gpuSkipped;return;}
  if(s.gpuTriangles+gpu.triangles>3000000||s.gpuVertices+gpu.vertices>3000000){lastOverflowReason=6;s.overflow=true;return;}
  if(gpuCutout){
   auto slots=field<void**>(prop,0xAC);auto resource=slots&&slots[0]?field<void*>(slots[0],0x24):nullptr;
   b.alpha=resource?field<IDirect3DBaseTexture9*>(resource,0x64):nullptr;
   if(!b.alpha){++s.skipped;return;}
   b.alpha->AddRef();b.threshold=blendOnly?0.f:field<unsigned char>(alpha,0x1A)/255.f;
   b.vertexAlpha=(field<unsigned>(prop,0x20)&8)!=0;++s.alpha;
  }
  b.pose=s.head.world(node);b.pose.p=b.pose.p-s.origin;b.gpu=int(s.gpuMeshes.size());
  s.gpuTriangles+=gpu.triangles;s.gpuVertices+=gpu.vertices;s.triangles+=gpu.triangles;s.vertices+=gpu.vertices;
  s.gpuMeshes.push_back(std::move(gpu));s.batches.push_back(std::move(b));if(!gpuCutout&&(field<unsigned>(prop,0x24)&6))++s.lod;return;
 }
 const float* grassIds=nullptr;void* grassTexture=nullptr;world_shadow_grass::InstanceData grass;
 if(type==4&&!world_shadow_grass::read(node,prop,data,s.origin,grass,grassIds,grassTexture)){++s.grassSkipped;return;}
 // Cache only immutable native static data. Skinned and dynamic meshes are
 // sampled again at the current native pose generation.
 auto dataType=field<unsigned>(data,0);bool immutable=!b.skinned&&(dataType==0x109DAC4||dataType==0x109D8AC||(type==4&&dataType==0x109FC54));
 auto found=cache.find(data);if(immutable&&found!=cache.end())b.mesh=found->second;
 if(!b.mesh){b.mesh=std::make_shared<Mesh>();auto& vertices=b.mesh->vertices;auto& indices=b.mesh->indices;vertices.resize(n);
 for(unsigned i=0;i<n;++i){vertices[i].p=positions[i];if(!finite(positions[i]))return;if(uv){vertices[i].u=uv[i*2];vertices[i].v=uv[i*2+1];}if(colours)vertices[i].a=colours[i*4+3];if(grassIds){float id=grassIds[i];if(!std::isfinite(id)||id<0||id>=228)return;vertices[i].instance=std::floor(id);}}
 auto vt=field<void**>(node,0);
 if(reinterpret_cast<void*(__thiscall*)(void*)>(vt[9])(node)){
  unsigned count=field<unsigned short>(data,0x40);auto ix=field<unsigned short*>(data,0x48);if(!count||!ix)return;indices.assign(ix,ix+count*3);
 }else if(reinterpret_cast<void*(__thiscall*)(void*)>(vt[8])(node)){
  unsigned count=field<unsigned short>(data,0x44);auto lengths=field<unsigned short*>(data,0x48);auto ix=field<unsigned short*>(data,0x4C);
  if(!count||count>4096||!lengths||!ix)return;
  for(unsigned strip=0;strip<count;++strip){unsigned len=lengths[strip];for(unsigned k=2;k<len;++k){unsigned short a=ix[k-2],c=ix[k-1],d=ix[k];if(a==c||c==d||d==a)continue;if(k&1)std::swap(a,c);indices.insert(indices.end(),{a,c,d});}ix+=len;}
 }else return;
 for(auto id:indices)if(id>=n){++s.skipped;return;}
 if(type==4){
  // Native grass duplicates a template to capacity. Validate that triangles
  // remain in instance order so only the active prefix needs a GPU draw.
  unsigned previous=0;
  for(unsigned i=0;i<indices.size();i+=3){
   unsigned id=unsigned(vertices[indices[i]].instance);
   if(id<previous||vertices[indices[i+1]].instance!=float(id)||vertices[indices[i+2]].instance!=float(id)){++s.grassSkipped;return;}
   b.mesh->grassEnds[id]=i+3;previous=id;
   b.mesh->grassVertexEnds[id]=std::max(b.mesh->grassVertexEnds[id],1u+std::max(unsigned(indices[i]),std::max(unsigned(indices[i+1]),unsigned(indices[i+2]))));
  }
  for(unsigned i=1;i<228;++i){b.mesh->grassEnds[i]=std::max(b.mesh->grassEnds[i],b.mesh->grassEnds[i-1]);b.mesh->grassVertexEnds[i]=std::max(b.mesh->grassVertexEnds[i],b.mesh->grassVertexEnds[i-1]);}
 }
 size_t bytes=vertices.size()*sizeof(Vertex)+indices.size()*2;
 if(immutable&&cache.size()<2048&&cacheBytes+bytes<96u*1024*1024){b.mesh->retained=data;scene::retain(data);cache[data]=b.mesh;cacheBytes+=bytes;}
 }
 if(b.mesh->indices.empty())return;
 b.indexCount=type==4?b.mesh->grassEnds[unsigned(grass.scale[3])-1]:unsigned(b.mesh->indices.size());
 if(!b.indexCount)return;
 unsigned liveVertices=type==4?b.mesh->grassVertexEnds[unsigned(grass.scale[3])-1]:n;
 // Node traversal and geometry bytes already bound this vector. A separate
 // 2,000-draw cutoff rejected valid, modest-size grass/mesh batches on reload.
 if(s.triangles-s.gpuTriangles+b.indexCount/3>1500000||s.vertices-s.gpuVertices+liveVertices>2000000){
  lastOverflowReason=s.vertices-s.gpuVertices+liveVertices>2000000?2:1;
  lastOverflowTriangles=s.triangles+b.indexCount/3;lastOverflowVertices=s.vertices+liveVertices;lastOverflowBatches=unsigned(s.batches.size());lastOverflowNodes=s.nodes;
  s.overflow=true;return;
 }
 if(b.skinned){if(!deform(node,b.mesh->vertices,s.head)){++s.skipped;return;}++s.skinned;
  Vec lo{1e20f,1e20f,1e20f},hi{-1e20f,-1e20f,-1e20f};
  for(auto& v:b.mesh->vertices){v.p=v.p-s.origin;lo={std::min(lo.x,v.p.x),std::min(lo.y,v.p.y),std::min(lo.z,v.p.z)};hi={std::max(hi.x,v.p.x),std::max(hi.y,v.p.y),std::max(hi.z,v.p.z)};}
  b.centre=(lo+hi)*.5f;b.radius=scene::distance(lo,hi)*.5f;b.pose={{{1,0,0},{0,1,0},{0,0,1}},{},1};
 }else{b.pose=s.head.world(node);b.pose.p=b.pose.p-s.origin;}
 if((af&512)||blendOnly||type==4){
  // Only validated PPLighting texture layout. Refuse an unbound alpha mask.
  if(type!=4&&type!=8&&type!=9){++s.skipped;return;}
  auto textures=type==4?nullptr:field<void**>(prop,0xAC);auto tex=type==4?grassTexture:(textures?textures[0]:nullptr);
  auto resource=tex?field<void*>(tex,0x24):nullptr;b.alpha=resource?field<IDirect3DBaseTexture9*>(resource,0x64):nullptr;
  if(!b.alpha||!uv){++s.skipped;return;}b.alpha->AddRef();b.threshold=blendOnly?0.f:(alpha?field<unsigned char>(alpha,0x1A)/255.f:.5f);++s.alpha;
 }
 if(type==4){b.grass=int(s.grasses.size());s.grasses.push_back(grass);}
 if(type==2||(type==8&&(field<unsigned>(prop,0x24)&6)))++s.lod;
 s.triangles+=b.indexCount/3;s.vertices+=liveVertices;s.batches.push_back(std::move(b));
}
inline void walk(void* node,Snapshot& s,unsigned depth=0){
 if(!node||depth>48||s.overflow)return;
 // The finite wrist beam uses loaded cell geometry. Distant LOD tiles can
 // overlap it even after their visible portions have been masked by the game.
 if(s.casterReach>0){
  const char* name=scene::name(node);
  if(!std::strcmp(name,"LandLOD"))return;
 }
 if(!s.visited.insert(node)){if(s.visited.full){lastOverflowReason=5;s.overflow=true;}return;}
 if(++s.nodes>30000){lastOverflowReason=4;lastOverflowNodes=s.nodes;s.overflow=true;return;}
 if((field<unsigned>(node,0x30)&1)&&!s.head.includeHidden(node))return;
 auto vt=field<void**>(node,0);
 if(reinterpret_cast<void*(__thiscall*)(void*)>(vt[7])(node)){geometry(node,s);return;}
 if(!reinterpret_cast<void*(__thiscall*)(void*)>(vt[3])(node))return;
 auto n=field<unsigned short>(node,0xA6);auto children=field<void**>(node,0xA0);if(n>4096||!children)return;
 for(unsigned i=0;i<n;++i)walk(children[i],s,depth+1);
}
// Same native ShadowSceneNode registry used by the verified wrist-light
// adapter. Read on the render thread; retain values, never native light pointers.
inline void roomLights(Snapshot& s){
 auto root=*reinterpret_cast<void**>(0x11F91C8);if(!root)return;
 struct Entry{Entry* next;Entry* previous;void* value;};
 float scores[2]{};unsigned entries=0;
 for(auto e=field<Entry*>(root,0xB4);e;e=e->next){
  // Registry corruption must not create an unbounded render-thread walk.
  if(++entries>1024)break;
  auto light=e->value?field<void*>(e->value,0xF8):nullptr;
  if(!light||field<unsigned char>(light,0x9D)!=2||!field<bool>(light,0x9C))continue;
  float radius=field<float>(light,0xE0),fade=field<float>(light,0xC4);
  Vec position=field<Vec>(light,0x8C),colour=field<Vec>(light,0xD4);
  if(!finite(position)||!finite(colour)||!std::isfinite(radius)||!std::isfinite(fade)||radius<32||radius>10000||fade<=.001f)continue;
  float distance=scene::distance(position,s.origin);if(distance>radius+600)continue;
  float luminance=std::max(0.f,.2126f*colour.x+.7152f*colour.y+.0722f*colour.z)*std::min(fade,4.f);
  float score=luminance/(1+distance*distance/(radius*radius));
  auto identity=reinterpret_cast<uintptr_t>(light);
  if(identity==previousRoomLights[0]||identity==previousRoomLights[1])score*=1.25f;
  RoomLight candidate{position,colour*std::min(fade,4.f),radius,identity};
  if(score>scores[0]){scores[1]=scores[0];s.room[1]=s.room[0];scores[0]=score;s.room[0]=candidate;}
  else if(score>scores[1]){scores[1]=score;s.room[1]=candidate;}
 }
 s.roomCount=scores[1]>.001f?2:(scores[0]>.001f?1:0);
 for(unsigned i=0;i<2;++i)previousRoomLights[i]=i<s.roomCount?s.room[i].identity:0;
}
inline bool collectUnsafe(Snapshot& s,Vec eye,bool lampActive,bool worldActive,float lampReach){
 // Sunlight covers the loaded scene. Only a finite local light has a reach.
 s.clear();s.origin=eye;s.casterReach=worldActive?0.f:lampReach;
 auto tes=*reinterpret_cast<void**>(0x11DEA10);auto sky=*reinterpret_cast<void**>(0x11DEA20);
 // TES retains the last exterior worldspace indoors. The player's actual cell
 // flag is authoritative (FNV TESObjectCELL flags0 +0x24, interior bit zero).
 auto player=*reinterpret_cast<void**>(0x11DEA3C);auto cell=scene::cell(player);
 bool exterior=cell&&!(field<unsigned char>(cell,0x24)&1);
 if(worldActive&&exterior&&tes&&field<void*>(tes,0x88)&&sky){auto sun=field<void*>(sky,0x28);auto light=sun?field<void*>(sun,0x1C):nullptr;
  if(light){s.sunDirection=field<Vec>(light,0xF0);s.sunColour=field<Vec>(light,0xD4);float len=scene::distance(s.sunDirection,{});
   s.sun=finite(s.sunColour)&&finite(s.sunDirection)&&len>.9f&&len<1.1f&&s.sunDirection.z<-.03f;
   if(s.sun)s.sunDirection=s.sunDirection*(1/len);
  }
 }
 if(worldActive&&!exterior)roomLights(s);else previousRoomLights[0]=previousRoomLights[1]=0;
 if(!s.sun&&!lampActive&&!s.roomCount)return true;
 auto graph=*reinterpret_cast<void**>(0x11DEB7C);if(!graph)return false;
 s.head.prepare(player);walk(graph,s);
 return !s.batches.empty()&&!s.overflow;
}
inline bool collect(Snapshot& s,Vec eye,bool lampActive,bool worldActive,float lampReach){
 __try{bool ok=collectUnsafe(s,eye,lampActive,worldActive,lampReach);if(!ok){if(s.overflow)++overflowFailures;else ++emptyFailures;}return ok;}
 __except(EXCEPTION_EXECUTE_HANDLER){++readFailures;return false;}
}
}
