#pragma once
#include <windows.h>
#include <string>
#include <vector>
namespace layer_discovery {
inline bool sameFile(const std::wstring& a,const std::wstring& b){
 if(!_wcsicmp(a.c_str(),b.c_str()))return true;
 auto open=[](const std::wstring& p){return CreateFileW(p.c_str(),0,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);};
 HANDLE first=open(a),second=open(b);BY_HANDLE_FILE_INFORMATION x{},y{};
 bool same=first!=INVALID_HANDLE_VALUE&&second!=INVALID_HANDLE_VALUE&&GetFileInformationByHandle(first,&x)&&GetFileInformationByHandle(second,&y)&&
  x.dwVolumeSerialNumber==y.dwVolumeSerialNumber&&x.nFileIndexHigh==y.nFileIndexHigh&&x.nFileIndexLow==y.nFileIndexLow;
 if(first!=INVALID_HANDLE_VALUE)CloseHandle(first);if(second!=INVALID_HANDLE_VALUE)CloseHandle(second);return same;
}
inline bool append(const wchar_t* name,const std::wstring& token,bool manifestDirectory=false){
 DWORD n=GetEnvironmentVariableW(name,nullptr,0);std::vector<wchar_t> old(n?n:1);
 if(n)GetEnvironmentVariableW(name,old.data(),n);std::wstring value=old.data();
 for(size_t start=0;start<value.size();){
  auto end=value.find(L';',start);auto entry=value.substr(start,end==std::wstring::npos?end:end-start);
  // MO2 can expose one manifest through both a real mod path and a virtual Data
  // path. File identity prevents registering the same layer through both names.
  if(entry==token||(manifestDirectory&&sameFile(entry+L"\\SeatedInput.json",token+L"\\SeatedInput.json")))return true;
  if(end==std::wstring::npos)break;start=end+1;
 }
 if(!value.empty())value+=L';';value+=token;return SetEnvironmentVariableW(name,value.c_str())!=0;
}
}
