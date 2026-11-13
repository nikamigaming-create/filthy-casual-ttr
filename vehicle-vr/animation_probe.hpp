#pragma once
#include <fstream>
#include <vector>
#include <string>
// Bounded local-only diagnostics; never enabled in a normal play profile.
namespace animation_probe {
inline bool captured=false;
inline bool read(void* address,void* target,size_t size){SIZE_T got=0;return address&&ReadProcessMemory(GetCurrentProcess(),address,target,size,&got)&&got==size;}
inline unsigned ptr(void* base,unsigned offset){unsigned p=0;read(static_cast<char*>(base)+offset,&p,4);return p;}
inline void capture(const std::string& folder,void* actor,void* renderedRoot){
 if(captured||folder.empty())return;captured=true;
 auto dir=folder+"\\animation-probe";CreateDirectoryA(dir.c_str(),nullptr);
 std::ofstream index(dir+"\\index.txt");index<<std::hex<<"actor "<<actor<<" renderedRoot "<<renderedRoot<<"\n";
 auto dump=[&](const std::string& name,void* address,size_t size){std::vector<char> data(size);if(read(address,data.data(),size)){std::ofstream out(dir+"\\"+name+".bin",std::ios::binary);out.write(data.data(),size);index<<name<<" "<<address<<" "<<size<<"\n";}};
 dump("matrix-column-writer",reinterpret_cast<void*>(0x4B4660),0x100);
 dump("native-quaternion-matrix",reinterpret_cast<void*>(0x4F0180),0x300);
 dump("multitarget-update",reinterpret_cast<void*>(0xA2FBE0),0x800);
 dump("player-getter",reinterpret_cast<void*>(0x950A10),0x180);dump("process-getter",reinterpret_cast<void*>(0x602150),0x100);
 dump("transform-update",reinterpret_cast<void*>(0xA3FDB0),0x600);dump("idle-play",reinterpret_cast<void*>(0x497F20),0x200);
 unsigned process=ptr(actor,0x68);unsigned vt=ptr(reinterpret_cast<void*>(process),0);dump("process-vtable",reinterpret_cast<void*>(vt),0x200);
 if(ptr(reinterpret_cast<void*>(vt),0x1B8)!=0x602150)return;
 for(int variant=0;variant<2;variant++){
  unsigned ad=variant?ptr(actor,0x690):ptr(reinterpret_cast<void*>(process),0x1C0);auto prefix=std::string(variant?"first":"third");
  dump(prefix+"-animdata",reinterpret_cast<void*>(ad),0x140);
  for(unsigned off:{0x124u,0x128u}){
   unsigned idle=ptr(reinterpret_cast<void*>(ad),off);auto id=prefix+std::to_string(off);dump(id+"-idle",reinterpret_cast<void*>(idle),0x40);
   unsigned seq=ptr(reinterpret_cast<void*>(idle),0x18);dump(id+"-sequence",reinterpret_cast<void*>(seq),0x90);
   dump(id+"-name",reinterpret_cast<void*>(ptr(reinterpret_cast<void*>(seq),8)),0x80);
   for(unsigned field:{0x14u,0x18u}){
    unsigned array=ptr(reinterpret_cast<void*>(seq),field);auto tag=id+"-array"+std::to_string(field);dump(tag,reinterpret_cast<void*>(array),0x1000);
    for(unsigned i=0;i<8;i++){
     unsigned item=ptr(reinterpret_cast<void*>(array),i*4);dump(tag+"-ptr"+std::to_string(i),reinterpret_cast<void*>(item),0x100);
     unsigned table=ptr(reinterpret_cast<void*>(item),0);dump(tag+"-vtable"+std::to_string(i),reinterpret_cast<void*>(table),0x100);
     unsigned update=ptr(reinterpret_cast<void*>(table),0x8C);dump(tag+"-update"+std::to_string(i),reinterpret_cast<void*>(update),0x500);
    }
   }
  }
 }
}
}
