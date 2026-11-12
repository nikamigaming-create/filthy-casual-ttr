#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include "../physical_reload_adapter.hpp"
__declspec(naked) void continuation(){__asm ret}
int main(){
 using namespace physical_reload_adapter;
 module=static_cast<unsigned char*>(VirtualAlloc(nullptr,0x1B6E000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));assert(module);
 auto old=reinterpret_cast<unsigned*>(module+0x121F13C),current=reinterpret_cast<unsigned*>(module+0x17EE0C4);
 *old=0xABCDEF01;*current=99;ejectBContinue=reinterpret_cast<void*>(&continuation);
 ejectBCommitted=true;ejectBStateEntry();assert(*current==4&&!ejectBCommitted&&*old==0xABCDEF01);
 *current=99;ejectBCommitted=false;ejectBStateEntry();assert(*current==5&&*old==0xABCDEF01);
 VirtualFree(module,0,MEM_RELEASE);module=nullptr;
 puts("PASS: real assembly branch writes TTR1.1.2 installed state4/5; old1.0 field untouched; commit flag cleared");
}
