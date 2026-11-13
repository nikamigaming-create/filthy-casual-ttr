#define NOMINMAX
#include <windows.h>
#include <cassert>
#include <cstdio>
#include "controls_runtime.hpp"
using namespace fc_controls;
int main(){
 Profile p,roundtrip;assert(decode(encode(p),roundtrip));assert(encode(p)==encode(roundtrip));assert(!decode("FilthyCasualControls 9\n",roundtrip));auto saved=encode(roundtrip);assert(!decode(encode(p)+"junk",roundtrip));assert(encode(roundtrip)==saved);
 std::ostringstream old;old<<"FilthyCasualControls 1\n";for(int i=0;i<24;i++)old<<i<<' '<<(i<19?i:7)<<" -1 0 600\n";Profile migrated;assert(decode(old.str(),migrated));assert(migrated.bindings[19].source==19&&migrated.bindings[20].source==20);for(int i=21;i<26;i++)assert(migrated.bindings[i].source==7);
 Frame f;for(auto&v:f)v.active=true;Engine e;e.profile.bindings[5]={7,-1,Level,600};e.update(f,0,false);f[7].value=.8f;e.update(f,10,false);assert(e.output[5].value==.8f);e.update(f,20,true);e.update(f,30,false);assert(e.output[5].value==0);f[7].value=0;e.update(f,40,false);f[7].value=1;e.update(f,50,false);assert(e.output[5].value==1);
 e.profile.bindings[5]={7,2,Hold,600};e.reset();f={};for(auto&v:f)v.active=true;e.update(f,100,false);f[7].value=f[2].value=1;e.update(f,200,false);e.update(f,799,false);assert(!e.output[5].value);e.update(f,800,false);assert(e.output[5].value==1);f[2].value=0;e.update(f,900,false);assert(!e.output[5].value);f[7].active=false;e.update(f,950,false);assert(!e.output[5].active);
 e.profile.bindings[19]={20,19,DoubleTap,450};e.reset();f={};for(auto&v:f)v.active=true;f[19].value=1;e.update(f,1000,false);f[20].value=1;e.update(f,1100,false);f[20].value=0;e.update(f,1180,false);f[20].value=1;e.update(f,1280,false);f[20].value=0;e.update(f,1360,false);assert(e.output[19].value==1);e.update(f,1370,false);assert(e.output[19].value==0);
 SetEnvironmentVariableA("SEATED_TEST_DIR","D:\\code\\ttwvr-seated-controls\\build\\controls-unit");
 using namespace controls_runtime;Frame raw;for(auto&v:raw)v.active=true;uint64_t tick=2000;auto frame=[&](){update(raw,tick+=100);};auto click=[&](int i){raw[i].value=1;frame();raw[i].value=0;frame();};
 frame();request();frame();assert(visible&&phase==0);selected=5;click(7);assert(phase==2);click(2);assert(phase==3&&proposal.source==2);click(7);assert(engine.profile.bindings[5].source==2);click(8);assert(!visible);raw[2].value=1;frame();Sample sample;assert(route(5,sample)&&sample.value==1);raw[2].value=0;frame();
 request();frame();selected=nativeCount;click(7);assert(phase==2);raw[19].value=1;frame();raw[20].value=1;frame();raw[19].value=raw[20].value=0;frame();assert(phase==3&&proposal.modifier>=0);click(7);assert(valid(engine.profile.bindings[nativeCount]));
 slot=8;auto before=read(path(slot));observed[slot]=true;originals[slot]=before;save();assert(read(path(slot))==encode(profiles));auto active=engine.profile;engine.profile=Profile{};load();assert(encode(engine.profile)==encode(active));assert(!dirty);
 atomicWrite(path(slot),"external change");save();assert(read(path(slot))=="external change");assert(status.find("changed on disk")!=std::string::npos);atomicWrite(path(slot),before);
 selected=4;engine.profile.bindings[4]={-2,-1,Level,600};visible=false;draining=false;Sample unbound{1,true,12};assert(route(4,unbound)&&unbound.active&&unbound.value==0);
 // A refused occupied-slot Save cannot authorize a second overwrite.
 slot=7;atomicWrite(path(slot),"occupied");observed[slot]=false;save();save();assert(read(path(slot))=="occupied");
 // Runtime loss clears latched custom inputs without replacing native defaults.
 engine.profile.bindings[5]={2,-1,Level,600};raw[2].value=1;frame();invalidate();Sample stopped{1,true,10};assert(route(5,stopped)&&!stopped.active&&stopped.value==0);Sample untouched{.7f,true,10};assert(!route(0,untouched)&&untouched.value==.7f);frame();assert(engine.output[5].value==0);raw[2].value=0;frame();raw[2].value=1;frame();assert(engine.output[5].value==1);raw[2].value=0;frame();
 Profiles pack,decoded;pack.get("vehicles.driving").bindings[5]={2,-1,Level,600};pack.get("vehicles.flight").bindings[5]={3,-1,Level,600};assert(decode(encode(pack),decoded));assert(decoded.get("vehicles.driving").bindings[5].source==2);assert(decoded.get("vehicles.flight").bindings[5].source==3);assert(decoded.get("onfoot").bindings[5].source==5);assert(!decode(encode(pack)+"junk",decoded));assert(decode(encode(p),decoded));assert(decoded.get("future.mode").bindings==p.bindings);
 // Editing an inactive mode never routes its bindings after closing.
 changeBank("vehicles.driving");engine.profile.bindings[5]={3,-1,Level,600};close();assert(bank=="onfoot");assert(profiles.get("vehicles.driving").bindings[5].source==3);assert(draining);
 visible=draining=false;engine.profile.bindings[0]={-1,-1,Level,600};Sample nativeDefault{.8f,true,20};assert(!route(0,nativeDefault)&&nativeDefault.value==.8f);available=false;assert(!route(0,nativeDefault));assert(jsonString("a\"b\\c\n")=="\"a\\\"b\\\\c\\u000a\"");
 Profiles many;for(int i=0;i<31;i++)many.get("absent."+std::to_string(i));auto currentProfiles=profiles;auto currentBank=bank;profiles=many;bank="new.mode";assert(!storeBank());assert(profiles.modes.size()==32);profiles=currentProfiles;bank=currentBank;
 puts("PASS: strict profiles, atomic save protection, analog, release barriers, holds, combos, double taps, live UI capture/apply/cancel, addon bindings, inactive input, unbind activity");
}
