#pragma once
#include "feature_setup.hpp"
#include <windows.h>
#include <string>
namespace fc_features {
inline bool read(const std::string& path,uint32_t& mask){
 if(GetPrivateProfileIntA("Setup","Version",0,path.c_str())!=1)return false;
 mask=0;for(int i=0;i<Count;++i)if(GetPrivateProfileIntA("Features",definitions[i].key,0,path.c_str()))mask|=bit(i);
 mask&=allowedMask;return true;
}
inline bool write(const std::string& path,uint32_t mask){
 const auto temp=path+".tmp";std::string text="[Setup]\r\nVersion=1\r\n\r\n[Features]\r\n";
 for(int i=0;i<Count;++i)text+=std::string(definitions[i].key)+"="+((mask&allowedMask&bit(i))?"1":"0")+"\r\n";
 auto file=CreateFileA(temp.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);if(file==INVALID_HANDLE_VALUE)return false;
 DWORD written=0;bool ok=WriteFile(file,text.data(),DWORD(text.size()),&written,nullptr)&&written==text.size()&&FlushFileBuffers(file);CloseHandle(file);
 if(ok)ok=MoveFileExA(temp.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
 if(!ok)DeleteFileA(temp.c_str());WritePrivateProfileStringA(nullptr,nullptr,nullptr,path.c_str());return ok;
}
}
