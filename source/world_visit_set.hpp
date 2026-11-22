#pragma once
#include <array>
#include <cstdint>
// Per-generation pointer membership. Fixed storage avoids allocating a hash
// node for every scene node on every frame. No native object is retained here.
struct WorldVisitSet {
 static constexpr unsigned capacity=65536;
 std::array<uintptr_t,capacity> keys{};
 std::array<unsigned,capacity> stamps{};
 unsigned epoch=1;bool full=false;
 void clear(){full=false;if(++epoch==0){stamps.fill(0);epoch=1;}}
 bool insert(void* pointer){
  auto key=reinterpret_cast<uintptr_t>(pointer);
  unsigned slot=unsigned(((key>>4)*2654435761u)^(key>>16))&(capacity-1);
  for(unsigned probe=0;probe<capacity;++probe,slot=(slot+1)&(capacity-1)){
   if(stamps[slot]!=epoch){keys[slot]=key;stamps[slot]=epoch;return true;}
   if(keys[slot]==key)return false;
  }
  full=true;return false;
 }
};
