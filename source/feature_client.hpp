#pragma once
#include <windows.h>
// Optional providers stay standalone when the core is absent. Read on DeferredInit,
// after NVSE has loaded every plugin. The core keeps addon decisions fixed per run.
namespace fc_feature_client {
inline bool enabled(const char* key){auto core=GetModuleHandleW(L"SeatedControls.dll");auto fn=core?reinterpret_cast<bool(__cdecl*)(const char*)>(GetProcAddress(core,"FC_FeatureEnabled")):nullptr;return !fn||fn(key);}
}
