#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <tlhelp32.h>
#include <wincrypt.h>
#include <io.h>
#include <fcntl.h>
#include <cstdio>
#include <cstddef>
#include <cstdarg>
#include <string>
#include <vector>
#include <set>
#include <mutex>
#include <atomic>
#include <algorithm>
#include "../nvse_api.hpp"

// Standalone diagnostics. No game offsets, detours, XR layers or game calls.
namespace {
HANDLE output=INVALID_HANDLE_VALUE,pipeRead=nullptr;
std::mutex outputMutex;
std::wstring root;
ULONGLONG started=0;
FILETIME startedFile{};
std::atomic<bool> initialized{false},stopping{false};
std::atomic<unsigned> events{0};
size_t written=0;
constexpr size_t cap=16*1024*1024;
std::string utf8(const std::wstring& s){int n=WideCharToMultiByte(CP_UTF8,0,s.data(),int(s.size()),nullptr,0,nullptr,nullptr);std::string out(n,0);if(n)WideCharToMultiByte(CP_UTF8,0,s.data(),int(s.size()),&out[0],n,nullptr,nullptr);return out;}
void write(const char* source,const char* data,size_t count){
 std::lock_guard<std::mutex> lock(outputMutex);if(output==INVALID_HANDLE_VALUE||written>=cap)return;
 char header[180];int n=sprintf_s(header,"\r\n[%llu ms] [%s]\r\n",GetTickCount64()-started,source);DWORD done;
 WriteFile(output,header,n,&done,nullptr);written+=done;
 count=std::min(count,cap-written);WriteFile(output,data,DWORD(count),&done,nullptr);written+=done;
 if(written>=cap){const char* note="\r\nSUPPORT LOG LIMIT REACHED. Further output discarded; gameplay continues.\r\n";WriteFile(output,note,DWORD(strlen(note)),&done,nullptr);}
 FlushFileBuffers(output);
}
void note(const char* fmt,...){char buf[8192];va_list a;va_start(a,fmt);int n=vsnprintf(buf,sizeof(buf),fmt,a);va_end(a);if(n>0)write("SUPPORT",buf,std::min(size_t(n),sizeof(buf)-1));}
std::wstring env(const wchar_t* name){DWORD n=GetEnvironmentVariableW(name,nullptr,0);if(!n||n>32768)return {};std::wstring s(n,0);DWORD used=GetEnvironmentVariableW(name,&s[0],n);if(used>=n)return {};s.resize(used);return s;}
std::string digest(const std::wstring& path){
 HANDLE f=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr);if(f==INVALID_HANDLE_VALUE)return "unreadable:"+std::to_string(GetLastError());
 HCRYPTPROV provider=0;HCRYPTHASH hash=0;std::string result="hash unavailable";
 if(CryptAcquireContextW(&provider,nullptr,nullptr,PROV_RSA_AES,CRYPT_VERIFYCONTEXT)&&CryptCreateHash(provider,CALG_SHA_256,0,0,&hash)){
  BYTE b[65536];DWORD n=0;bool ok=true;while(ReadFile(f,b,sizeof(b),&n,nullptr)&&n){if(!CryptHashData(hash,b,n,0)){ok=false;break;}}
  BYTE d[32];DWORD length=sizeof(d);if(ok&&CryptGetHashParam(hash,HP_HASHVAL,d,&length,0)){result.clear();char hex[3];for(DWORD i=0;i<length;i++){sprintf_s(hex,"%02X",d[i]);result+=hex;}}
 }
 if(hash)CryptDestroyHash(hash);if(provider)CryptReleaseContext(provider,0);CloseHandle(f);return result;
}
void environment(){
 for(auto name:{L"XR_RUNTIME_JSON",L"XR_API_LAYER_PATH",L"XR_ENABLE_API_LAYERS",L"XR_DISABLE_API_LAYERS",L"XR_LOADER_DEBUG"})note("%s=%s",utf8(name).c_str(),utf8(env(name)).c_str());
 for(auto hive:{HKEY_CURRENT_USER,HKEY_LOCAL_MACHINE})for(REGSAM view:{KEY_WOW64_32KEY,KEY_WOW64_64KEY}){
  HKEY key=nullptr;LONG result=RegOpenKeyExW(hive,L"SOFTWARE\\Khronos\\OpenXR\\1",0,KEY_READ|view,&key);
  if(result==ERROR_SUCCESS){wchar_t value[2048]{};DWORD type=0,bytes=sizeof(value)-sizeof(wchar_t);result=RegQueryValueExW(key,L"ActiveRuntime",nullptr,&type,reinterpret_cast<BYTE*>(value),&bytes);note("Registry %s %s ActiveRuntime (%ld): %s",hive==HKEY_CURRENT_USER?"HKCU":"HKLM",view==KEY_WOW64_32KEY?"32":"64",result,utf8(value).c_str());RegCloseKey(key);}
 }
}
DWORD WINAPI capture(void*){char buffer[8192];DWORD n=0;while(ReadFile(pipeRead,buffer,sizeof(buffer),&n,nullptr)&&n)write("XR / process stdout+stderr",buffer,n);return 0;}
void redirect(){
 HANDLE pipeWrite=nullptr;SECURITY_ATTRIBUTES sa{sizeof(sa),nullptr,TRUE};
 if(!CreatePipe(&pipeRead,&pipeWrite,&sa,65536)){note("Capture pipe failed: %lu",GetLastError());return;}
 SetHandleInformation(pipeRead,HANDLE_FLAG_INHERIT,0);
 HANDLE reader=CreateThread(nullptr,0,capture,nullptr,0,nullptr);if(!reader){CloseHandle(pipeRead);CloseHandle(pipeWrite);note("Capture thread failed");return;}CloseHandle(reader);
 int fd=_open_osfhandle(reinterpret_cast<intptr_t>(pipeWrite),_O_BINARY|_O_WRONLY);
 if(fd<0){CloseHandle(pipeWrite);note("Capture CRT handle failed");return;}
 fflush(stdout);fflush(stderr);int a=_dup2(fd,1),b=_dup2(fd,2);_close(fd);
 if(a==0)SetStdHandle(STD_OUTPUT_HANDLE,reinterpret_cast<HANDLE>(_get_osfhandle(1)));
 if(b==0)SetStdHandle(STD_ERROR_HANDLE,reinterpret_cast<HANDLE>(_get_osfhandle(2)));
 setvbuf(stdout,nullptr,_IONBF,0);setvbuf(stderr,nullptr,_IONBF,0);
 note("Process-local capture installed: stdout=%d stderr=%d. No runtime selection changed.",a,b);
 // Tests the exact stdout/stderr route used by the loader; not an XR success claim.
 const char* check="Filthy Casuals support capture self-check\n";DWORD checked=0;
 WriteFile(GetStdHandle(STD_ERROR_HANDLE),check,DWORD(strlen(check)),&checked,nullptr);
}
struct Source {std::wstring relative;DWORD offset=0;bool first=true;};
std::vector<Source> sources;
void collect(Source& s,bool settings=false){
 auto path=root+s.relative;HANDLE f=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr);if(f==INVALID_HANDLE_VALUE)return;
 FILETIME time{};GetFileTime(f,nullptr,nullptr,&time);DWORD size=GetFileSize(f,nullptr);
 // Ignore logs from earlier sessions, but include persistent settings.
 if(!settings&&CompareFileTime(&time,&startedFile)<0){CloseHandle(f);return;}
 if(size==INVALID_FILE_SIZE||size>64*1024*1024){CloseHandle(f);return;}
 if(size<s.offset)s.offset=0;
 if(size>s.offset){
  DWORD begin=s.offset;if(size-begin>512*1024){begin=size-512*1024;note("%s: retaining latest 512 KiB of burst",utf8(s.relative).c_str());}
  SetFilePointer(f,begin,nullptr,FILE_BEGIN);std::vector<char> bytes(size-begin);DWORD n=0;if(ReadFile(f,bytes.data(),DWORD(bytes.size()),&n,nullptr)&&n){write(utf8(s.relative).c_str(),bytes.data(),n);s.offset=begin+n;}
 }else if(settings&&s.first)note("%s: empty",utf8(s.relative).c_str());
 s.first=false;CloseHandle(f);
}
bool relevant(std::wstring path){std::transform(path.begin(),path.end(),path.begin(),towlower);return path.find(L"\\nvse\\plugins\\")!=std::wstring::npos||path.find(L"openxr")!=std::wstring::npos||path.find(L"openvr")!=std::wstring::npos||path.find(L"vrclient")!=std::wstring::npos||path.find(L"virtualdesktop")!=std::wstring::npos||path.find(L"oculus")!=std::wstring::npos||path.find(L"nvse_")!=std::wstring::npos||path.find(L"falloutnv.exe")!=std::wstring::npos;}
void modules(std::set<std::wstring>& seen){
 HANDLE snap=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE,GetCurrentProcessId());if(snap==INVALID_HANDLE_VALUE)return;
 MODULEENTRY32W m{};m.dwSize=sizeof(m);if(Module32FirstW(snap,&m))do{
  std::wstring path=m.szExePath;if(relevant(path)&&seen.insert(path).second)note("Loaded module: %s | SHA256=%s",utf8(path).c_str(),digest(path).c_str());
 }while(Module32NextW(snap,&m));CloseHandle(snap);
}
DWORD WINAPI worker(void*){
 environment();
 for(auto relative:{L"Data\\NVSE\\Plugins\\SeatedControls.ini",L"Data\\NVSE\\Plugins\\SeatedControls.ini.features.ini",L"Data\\NVSE\\Plugins\\SeatedControls\\SeatedInput.json"}){Source s{relative};collect(s,true);}
 for(auto relative:{L"Data\\NVSE\\Plugins\\TTWVR.dll",L"Data\\NVSE\\Plugins\\SeatedControls.dll",L"Data\\NVSE\\Plugins\\SeatedControls\\SeatedInput.dll"})note("File: %s | SHA256=%s",utf8(relative).c_str(),digest(root+relative).c_str());
 for(auto name:{L"SeatedControls.log",L"nvse.log",L"nvse_loader.log",L"nvse_steam_loader.log",L"falloutnv_error.log"})sources.push_back({name});
 std::set<std::wstring> seen;unsigned ticks=0;
 do{
  unsigned changed=events.exchange(0);if(changed){note("NVSE event mask=%08X",changed);environment();}
  for(auto&s:sources)collect(s);
  if(ticks++%5==0)modules(seen);
  if(stopping.load())break;
  Sleep(1000);
 }while(true);
 note("Normal exit observed. Diagnostic capture complete.");return 0;
}
void __cdecl message(Message* m){if(m&&m->type<32&&m->type!=MainLoop)events.fetch_or(1u<<m->type);if(m&&(m->type==ExitGame||m->type==7)){stopping=true;note("NVSE exit received (type=%u). Latest log chunks already flushed.",m->type);}}
bool begin(){
 if(initialized.exchange(true))return output!=INVALID_HANDLE_VALUE;
 started=GetTickCount64();GetSystemTimeAsFileTime(&startedFile);
 // The NVSE launcher finishes writing before this plugin loads. Include logs
 // from process creation, not only those modified after our Query callback.
 FILETIME ended{},kernel{},user{};GetProcessTimes(GetCurrentProcess(),&startedFile,&ended,&kernel,&user);
 wchar_t exe[32768]{};GetModuleFileNameW(nullptr,exe,32768);root=exe;root=root.substr(0,root.find_last_of(L"\\/")+1);
 auto path=root+L"FilthyCasuals-Support.log";output=CreateFileW(path.c_str(),GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
 if(output==INVALID_HANDLE_VALUE)return false;
 note("Filthy Casuals Support 0.1.0 | process=%lu | x86\r\nOne-run diagnostics. This file replaces the previous capture on each launch.\r\nIncludes local file paths, settings and logs. No saves/binaries uploaded or copied.\r\nGame: %s",GetCurrentProcessId(),utf8(exe).c_str());
 note("XR_LOADER_DEBUG before=%s",utf8(env(L"XR_LOADER_DEBUG")).c_str());
 SetEnvironmentVariableW(L"XR_LOADER_DEBUG",L"all");redirect();
 if(GetModuleHandleW(L"openxr_loader.dll"))note("OpenXR loader already mapped at diagnostic startup; earliest loader messages may be unavailable.");
 return true;
}
}
extern "C" __declspec(dllexport) bool __cdecl NVSEPlugin_Query(const NVSE* nvse,PluginInfo* info){
 if(!nvse||!info)return false;info->infoVersion=1;info->name="Filthy Casuals Support";info->version=1;
 if(nvse->isEditor||nvse->runtime!=0x040020D0)return false;return begin();
}
extern "C" __declspec(dllexport) bool __cdecl NVSEPlugin_Load(const NVSE* nvse){
 if(!begin())return false;note("NVSE runtime=%08X version=%u",nvse->runtime,nvse->version);
 auto messages=static_cast<Messaging*>(nvse->queryInterface(2));if(messages)messages->listen(nvse->getPluginHandle(),"NVSE",message);
 HANDLE thread=CreateThread(nullptr,0,worker,nullptr,0,nullptr);if(!thread){note("Collector worker failed: %lu",GetLastError());return false;}CloseHandle(thread);return true;
}
