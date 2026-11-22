#define NOMINMAX
#include <windows.h>
#include <cassert>
#include <cstdio>
#include "controls_runtime.hpp"
using namespace fc_controls;
int main(){
 controls_ini::Config c;assert(controls_ini::decode("[Bindings]\nMenuButton=RightStickClick\nOpenControls=RightGrip+LeftGrip,Hold,800\nWeaponWheel=RightPrimary,DoubleTap,450\n[Bindings.vehicles.driving]\nMoveForward=LeftTrigger\n",c));
 Profile p;c.apply("onfoot",p);assert(p.bindings[10].source==9&&p.bindings[21].source==7&&p.bindings[21].gesture==DoubleTap);c.apply("vehicles.driving",p);assert(p.bindings[14].source==0);
 auto keep=c;for(auto bad:{"[Bindings]\nMenuButton=MadeUpButton\n","[Bindings]\nOpenControls=LeftGrip+LeftGrip,Hold,800\n","[Bindings]\nMenuButton=RightStickClick,Hold,0\n","[Bindings]\nWrongAction=LeftGrip\n","[Bindings]\nMenuButton=RightGrip\nMenuButton=LeftGrip\n"}){assert(!controls_ini::decode(bad,c));assert(c.editor==keep.editor&&c.global==keep.global);}
 Binding b;for(int i=0;i<sourceCount;i++){assert(controls_ini::parse(sourceNames[i],b));assert(b.source==i);}assert(controls_ini::parse("right_stick_click + left_grip, hold, 1000",b)&&b.source==9&&b.modifier==1);
 using namespace controls_runtime;
 SetEnvironmentVariableA("SEATED_TEST_DIR","D:\\code\\ttwvr-seated-controls\\build\\controls-ini-unit");std::filesystem::create_directories("D:\\code\\ttwvr-seated-controls\\build\\controls-ini-unit");
 assert(atomicWrite("D:\\code\\ttwvr-seated-controls\\build\\controls-ini-unit\\controls-test.ini","[Bindings]\nMenuButton=RightStickClick\nOpenControls=RightGrip+LeftGrip,Hold,800\nWeaponWheel=RightPrimary,DoubleTap,450\n"));
 Frame f;for(int i=0;i<19;i++)f[i].active=true;f[10].active=false;uint64_t now=100;auto step=[&](){update(f,now+=100);};step();step();assert(engine.profile.bindings[10].source==9);
 f[9].value=1;step();Sample absentMenu{0,false,0};assert(route(10,absentMenu)&&absentMenu.active&&absentMenu.value==1);f[9].value=0;step();
 f[1].value=f[6].value=1;step();for(int i=0;i<8;i++)step();assert(visible);f[1].value=f[6].value=0;step();assert(!draining);f[8].value=1;step();f[8].value=0;step();assert(!visible);
 // Changes remain usable across mode changes; explicit INI reapplies on Load.
 engine.profile.bindings[21]={3,-1,Tap,450};changeBank("vehicles.driving");assert(engine.profile.bindings[21].source==7);changeBank("onfoot");assert(engine.profile.bindings[21].source==3);
 slot=6;originals[slot]=read(path(slot));observed[slot]=true;save();load();assert(engine.profile.bindings[21].source==7);
 // No Menu or capacitive sensors required, including only one active controller.
 f={};f[5].active=f[6].active=f[8].active=true;startupEditorPending=true;step();assert(visible);f[8].value=1;step();f[8].value=0;step();assert(!visible);step();assert(!visible);
 puts("PASS: typed inputs, malformed-INI rejection, per-mode overrides, missing Menu/touch sensors, configurable editor access, startup fallback, profile precedence");
}
