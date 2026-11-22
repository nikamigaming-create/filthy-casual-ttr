#pragma once
// Private simulator diagnostic. No pose mutations and no save data.
namespace field_rig_debug {
inline void dump(std::ostream& out,void* node,bool& first,int depth=0){
 if(!node||depth>48)return;
 const char* name=scene::name(node);
 if(strstr(name,"Bip01")||strstr(name,"WorldRoot")||strstr(name,"FCBook")||strstr(name,"FC_ControlsBook")||strstr(name,"Weapon")||strstr(name,"Head")||strstr(name,"head")||strstr(name,"Hair")||strstr(name,"Eye")||strstr(name,"Teeth")||strstr(name,"Face")){
  if(!first)out<<",";first=false;out<<"{\"name\":\""<<name<<"\",\"parent\":\""<<scene::name(scene::parent(node))<<"\"";
  auto write=[&](const char* key,const scene::Transform& t){out<<",\""<<key<<"\":[";for(int i=0;i<9;i++){if(i)out<<",";out<<t.r[i/3][i%3];}out<<","<<t.p.x<<","<<t.p.y<<","<<t.p.z<<","<<t.scale<<"]";};
  out<<",\"flags\":"<<scene::field<unsigned>(node,0x30);write("local",scene::local(node));write("world",scene::world(node));out<<"}";
 }
 auto vt=scene::field<void**>(node,0);if(!reinterpret_cast<void*(__thiscall*)(void*)>(vt[3])(node))return;
 auto count=scene::field<unsigned short>(node,0xA6);auto children=scene::field<void**>(node,0xA0);if(!children||count>4096)return;
 for(unsigned i=0;i<count;i++)dump(out,children[i],first,depth+1);
}
}
