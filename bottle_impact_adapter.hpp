#pragma once
#include "scene_adapter.hpp"
#include "bottle_impact.hpp"
// FNV 1.4.0.525 Havok layouts/vtables from JIP LN internal/havok.h and
// class_vtbls.h. Read on the game thread, only for the tracked bottle reference.
namespace bottle_impact_adapter {
inline bool readable(void* p,size_t n){return p&&!IsBadReadPtr(p,n);}
inline void* body(void* node,int depth=0){
 if(!readable(node,0x9C)||depth>24)return nullptr;
 auto c=scene::field<void*>(node,0x1C);auto wrapper=readable(c,0x14)?scene::field<void*>(c,0x10):nullptr;
 auto b=readable(wrapper,0xC)?scene::field<void*>(wrapper,8):nullptr;
 if(readable(b,0x220)&&scene::field<unsigned char>(b,0x28)==1)return b;
 auto vt=scene::field<void**>(node,0);if(!readable(vt,16)||!reinterpret_cast<void*(__thiscall*)(void*)>(vt[3])(node)||!readable(node,0xAC))return nullptr;
 auto count=scene::field<unsigned short>(node,0xA6);auto children=scene::field<void**>(node,0xA0);
 if(count>256||!readable(children,count*sizeof(void*)))return nullptr;
 for(unsigned i=0;i<count;i++)if(auto found=body(children[i],depth+1))return found;return nullptr;
}
inline bool contact(void* instance,void* other,void*& surface){
 if(!readable(instance,0x1C)||!readable(other,0x220))return false;
 auto data=scene::field<void*>(instance,0xC);
 if(!readable(data,4)||scene::field<uintptr_t>(data,0)!=0x10CC934)return false;
 surface=other;return true;
}
inline bottle_impact::Vec velocity(void* b){auto v=scene::field<scene::Vec>(b,0x1B0);return {v.x*.1f,v.y*.1f,v.z*.1f};}
inline bottle_impact::Sample sample(void* root,bool held,double seconds,void** contactBody=nullptr){
 bottle_impact::Sample s;auto b=body(root);if(!b||!scene::field<void*>(b,8))return s;
 s.valid=true;s.held=held;s.body=reinterpret_cast<uintptr_t>(b);s.seconds=seconds;s.velocity=velocity(b);
 if(!std::isfinite(bottle_impact::length(s.velocity))){s.valid=false;return s;}
 void* surface=nullptr;auto entries=scene::field<unsigned char*>(b,0xAC);auto count=scene::field<unsigned short>(b,0xB0);
 if(count<=128&&readable(entries,count*0x30))for(unsigned i=0;i<count&&!surface;i++){auto p=entries+i*0x30;auto other=scene::field<void*>(p,8);if(other==b)other=scene::field<void*>(p,4);if(other!=b)contact(scene::field<void*>(p,0),other,surface);}
 auto constraints=scene::field<void**>(b,0xB4);auto n=scene::field<unsigned>(b,0xB8);
 if(n<=128&&readable(constraints,n*sizeof(void*)))for(unsigned i=0;i<n&&!surface;i++){auto p=constraints[i];if(readable(p,0x1C)){auto other=scene::field<void*>(p,0x14);if(other==b)other=scene::field<void*>(p,0x18);if(other!=b)contact(p,other,surface);}}
 if(contactBody)*contactBody=surface;s.contact=surface!=nullptr;if(surface)s.surfaceVelocity=velocity(surface);return s;
}
}
