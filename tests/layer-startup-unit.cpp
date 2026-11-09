#define WIN32_LEAN_AND_MEAN
#include "../layer_discovery.hpp"
#include <cassert>
#include <cstdio>
#include <cstring>
#include "openxr.h"
#include "../gesture_input.hpp"
int main(){
 // Different paths to the same manifest must not add a second loader entry.
 wchar_t temp[MAX_PATH]{};GetTempPathW(MAX_PATH,temp);
 std::wstring root=std::wstring(temp)+L"fc-layer-test-"+std::to_wstring(GetCurrentProcessId());
 auto a=root+L"\\actual",b=root+L"\\alias",c=root+L"\\other";
 assert(CreateDirectoryW(root.c_str(),nullptr));
 for(auto dir:{a,b,c})assert(CreateDirectoryW(dir.c_str(),nullptr));
 auto write=[](const std::wstring& path){HANDLE f=CreateFileW(path.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);assert(f!=INVALID_HANDLE_VALUE);CloseHandle(f);};
 auto file=[](const std::wstring& dir){return dir+L"\\SeatedInput.json";};
 write(file(a));write(file(c));assert(CreateHardLinkW(file(b).c_str(),file(a).c_str(),nullptr));
 assert(layer_discovery::sameFile(file(a),file(b)));
 assert(!layer_discovery::sameFile(file(a),file(c)));
 const wchar_t* env=L"FC_LAYER_UNIT_PATH";
 assert(SetEnvironmentVariableW(env,(c+L";"+a).c_str()));
 assert(layer_discovery::append(env,b,true));wchar_t value[4096]{};GetEnvironmentVariableW(env,value,4096);
 assert(std::wstring(value)==c+L";"+a); // unrelated discovery paths preserved
 SetEnvironmentVariableW(env,nullptr);
 for(auto dir:{a,b,c}){assert(DeleteFileW(file(dir).c_str()));assert(RemoveDirectoryW(dir.c_str()));}assert(RemoveDirectoryW(root.c_str()));
 // Repeated creation callbacks retain an existing live action set, even if an
 // optional attach failed. No calls through null GIPA, reset or duplicate set.
 gesture_input::instance=XrInstance(1);gesture_input::set=XrActionSet(2);gesture_input::ready=true;
 gesture_input::init(nullptr,XrInstance(1),nullptr,nullptr);
 assert(gesture_input::set==XrActionSet(2)&&gesture_input::ready);
 gesture_input::ready=false;gesture_input::init(nullptr,XrInstance(1),nullptr,nullptr);
 assert(gesture_input::set==XrActionSet(2));
 gesture_input::initializing=true;gesture_input::init(nullptr,XrInstance(3),nullptr,nullptr);
 assert(gesture_input::instance==XrInstance(1));
 puts("PASS: manifest aliases, unrelated paths, repeat initialization, re-entry");
}
