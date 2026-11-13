#pragma once
#include "controls_provider.hpp"
#include "controls_model.hpp"
#include <tlhelp32.h>
#include <vector>
namespace controls_catalog {
struct Mode {std::string id,title,description;std::array<std::string,fc_controls::actionCount> labels;const fc_provider::Provider* provider=nullptr;unsigned index=0;bool available=true;};
inline std::vector<Mode> modes;
inline uint64_t scanned=0;
inline Mode native(){Mode m;m.id="onfoot";m.title="ON FOOT";m.description="Native TTR contexts share input channels; spatial interactions stay native.";for(int i=0;i<fc_controls::actionCount;i++)m.labels[i]=fc_controls::actionNames[i];return m;}
inline void discover(uint64_t now){if(modes.empty())modes.push_back(native());if(scanned&&now-scanned<1000)return;scanned=now;
 auto snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE,GetCurrentProcessId());if(snapshot==INVALID_HANDLE_VALUE)return;MODULEENTRY32W e{};e.dwSize=sizeof(e);
 if(Module32FirstW(snapshot,&e))do{auto query=reinterpret_cast<fc_provider::Query>(GetProcAddress(e.hModule,"FC_ControlsProviderV1"));if(!query)continue;const auto* p=query();if(!p||p->size!=sizeof(fc_provider::Provider)||p->abi!=1||!p->modes||!p->available||!p->current||p->count>16)continue;
 for(unsigned i=0;i<p->count&&modes.size()<32;i++){auto& src=p->modes[i];if(!src.id||!src.title||!fc_controls::validModeId(src.id))continue;bool exists=false;for(auto& m:modes)if(m.id==src.id){exists=true;break;}if(exists)continue;Mode m;m.id=src.id;m.title=src.title;m.description=src.description?src.description:"";m.provider=p;m.index=i;for(int j=0;j<fc_controls::actionCount;j++)m.labels[j]=src.labels[j]?src.labels[j]:"Unused channel";modes.push_back(m);}
 }while(Module32NextW(snapshot,&e));CloseHandle(snapshot);
 for(auto& m:modes)if(m.provider)m.available=(m.provider->available()&(1u<<m.index))!=0;
}
inline std::string current(){for(auto& m:modes)if(m.available&&m.provider&&m.provider->current()==int(m.index))return m.id;return "onfoot";}
inline const Mode& find(const std::string& id){for(auto& m:modes)if(m.id==id)return m;return modes.front();}
}
