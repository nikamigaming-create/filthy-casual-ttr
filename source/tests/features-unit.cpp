#include "../feature_setup.hpp"
#include "../feature_store.hpp"
#include "../world_visit_set.hpp"
#include <cassert>
#include <cstdio>
#include <string>
int main(){using namespace fc_features;
 static WorldVisitSet visited;
 for(unsigned i=1;i<=WorldVisitSet::capacity;++i)assert(visited.insert(reinterpret_cast<void*>(uintptr_t(i)*16)));
 assert(!visited.insert(reinterpret_cast<void*>(16))&&!visited.full);
 assert(!visited.insert(reinterpret_cast<void*>(uintptr_t(WorldVisitSet::capacity+1)*16))&&visited.full);
 visited.clear();assert(!visited.full&&visited.insert(reinterpret_cast<void*>(16)));
 visited.epoch=~0u;visited.clear();assert(visited.epoch==1&&visited.insert(reinterpret_cast<void*>(16)));
 Selection s;s.begin(0,coreMask);assert(!s.pending);assert(!s.toggle(Pool));assert(!s.toggle(Workshop));assert(!s.toggle(Command));assert(!s.toggle(Driving));
 assert(s.toggle(Weapons)&&s.pending==bit(Weapons));assert(s.toggle(Remapping));assert(s.pending&bit(Remapping));s.clear();assert(!s.pending);
 s.begin(allowedMask,allowedMask);assert(s.pending&bit(Pool));assert(s.toggle(Pool));assert(!(s.pending&bit(Pool)));assert(s.toggle(Driving));assert(!(s.pending&bit(Driving)));
 for(int i=0;i<Count;++i)assert(find(definitions[i].key)==i);assert(find("Unknown")==-1);
 for(int id=0;id<Count;++id){s.begin(0,allowedMask);bool available=bool(allowedMask&bit(id));assert(s.toggle(id)==available);assert(s.pending==(available?bit(id):0));if(available){assert(s.toggle(id));assert(!s.pending);}}
 for(int a=0;a<Count;++a)for(int b=0;b<Count;++b){s.begin(allowedMask,allowedMask);s.toggle(a);s.toggle(b);uint32_t expected=allowedMask;if(allowedMask&bit(a))expected^=bit(a);if(allowedMask&bit(b))expected^=bit(b);assert(s.pending==expected);}
 char temp[MAX_PATH];GetTempPathA(MAX_PATH,temp);std::string file=std::string(temp)+"fc-features-unit-"+std::to_string(GetCurrentProcessId())+".ini";
 uint32_t mask=123;assert(!read(file,mask));assert(write(file,bit(Weapons)|bit(Mouth)|bit(Remapping)|bit(Pool)));assert(read(file,mask));assert(mask==(bit(Weapons)|bit(Mouth)|bit(Remapping)|bit(Pool)));
 assert(WritePrivateProfileStringA("Features","Flight","1",file.c_str()));
 assert(WritePrivateProfileStringA("Features","Riding","1",file.c_str()));
 assert(read(file,mask)&&!(mask&(bit(Flight)|bit(Riding))));
 assert(write(file,0));assert(read(file,mask)&&mask==0);assert(!write(file+"/missing/file",allowedMask));assert(read(file,mask)&&mask==0);DeleteFileA(file.c_str());
 puts("PASS: fresh defaults, isolated choices, locked features, missing addons, persistence and failed-write preservation");
}
