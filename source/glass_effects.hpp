#pragma once
#include "glass_decal_adapter.hpp"
#include "glass_variation.hpp"
// Shared by fragile bottles and vehicle panes. The caller owns destruction;
// this function never disables/deletes an item, actor, or collision body.
namespace glass_effects {
struct Result {bool decal=false,particles=false,sound=false;};
inline Result emit(scene::Vec point,scene::Vec normal,void* surface,void* excludedRoot,
                   void* texture,unsigned seed,float impact,bool requireDecal=false){
 auto style=glass_variation::choose(seed,impact);
 auto node=glass_decal_adapter::raySurface(point,normal,excludedRoot);
 if(!node)node=glass_decal_adapter::parentNode(surface);
 auto cell=glass_decal_adapter::impactCell(point);
 Result out;
 out.decal=glass_decal_adapter::add(point,normal,node,cell,texture,excludedRoot,style.width,style.height);
 if(requireDecal&&!out.decal)return out;
 out.particles=glass_decal_adapter::puff(point,normal,cell);
 out.sound=glass_decal_adapter::smashSound(point);
 return out;
}
}
