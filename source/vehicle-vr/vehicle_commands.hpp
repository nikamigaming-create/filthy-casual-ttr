#pragma once
// xNVSE public CommandInfo/CommandTable prefix. Preserve original execution and
// bytecode advancement; override only queries made by the currently driven car.
#define ARGS void* params,void* data,Form* object,Form* container,Script* script,void* events,double* result,unsigned* offset
#define PASS params,data,object,container,script,events,result,offset
using Exec=bool(__cdecl*)(ARGS);
struct Command {const char* name;const char* shortName;unsigned opcode;const char* help;unsigned short parent,count;void* params;Exec execute;void* parse;void* eval;unsigned flags;};
static_assert(offsetof(Command,execute)==0x18 && sizeof(Command)==0x28);
struct Table {unsigned version;void* start;void* end;void* opcode;const Command*(__cdecl* byName)(const char*);};
Table* commands{};Exec originals[7]{};
bool ownsVehicleQuery(Form* object,Script* script){
 if(!(requestedModes.load()&mountedMode()) || !active || riding || !car || object!=car || !script)return false;
 return reinterpret_cast<Form*>(script)==(flying?flightScript:drivingScript);
}
bool __cdecl steer(ARGS){bool ok=originals[0](PASS);if(ok&&ownsVehicleQuery(object,script))*result=flying?flightX:steering;return ok;}
bool __cdecl gas(ARGS){bool ok=originals[1](PASS);if(ok&&ownsVehicleQuery(object,script))*result=arcadeOwns&&!flying?0:throttle;return ok;}
bool __cdecl stop(ARGS){bool ok=originals[2](PASS);if(ok&&ownsVehicleQuery(object,script))*result=arcadeOwns&&!flying?0:brake;return ok;}
bool __cdecl fore(ARGS){bool ok=originals[3](PASS);if(ok&&flying&&ownsVehicleQuery(object,script))*result=flightY;return ok;}
bool __cdecl buttons(ARGS){auto start=*offset;bool ok=originals[4](PASS);if(ok&&flying&&ownsVehicleQuery(object,script)){
 unsigned mask=0,flags=0;using Extract=bool(__cdecl*)(void*,void*,unsigned*,Script*,void*,...);
 if(reinterpret_cast<Extract>(scripts->extract)(params,data,&start,script,events,&mask,&flags))*result=(flightButtons&mask)?1:0;
 }return ok;}
// The desktop flight script aims the entire aircraft with the player's head.
// In VR, physical head motion is free look. Only this specific script receives
// the commanded aircraft heading and neutral pitch; other GetAngle users pass.
bool __cdecl flightAngle(ARGS){auto start=*offset;bool ok=originals[5](PASS);
 if(ok&&(requestedModes.load()&FlightMode)&&active&&flying&&object==player&&reinterpret_cast<Form*>(script)==flightScript){
  unsigned axisCode=0;using Extract=bool(__cdecl*)(void*,void*,unsigned*,Script*,void*,...);
  if(reinterpret_cast<Extract>(scripts->extract)(params,data,&start,script,events,&axisCode)){
   if(axisCode=='X')*result=flightPitch/1.8f;else if(axisCode=='Z')*result=flightHeading;
  }
 }return ok;
}
// The upstream flight script disables fighting every frame, which forcibly
// holsters a native weapon. Preserve its other default seat locks without
// touching any other script or explicitly customized DisablePlayerControls call.
bool __cdecl flightLocks(ARGS){
 if((requestedModes.load()&FlightMode)&&active&&flying&&reinterpret_cast<Form*>(script)==flightScript&&flightControlLocks){
  auto start=*offset;unsigned move=1,pip=1,fight=1,pov=1,look=0,text=0,sneak=0;
  using Extract=bool(__cdecl*)(void*,void*,unsigned*,Script*,void*,...);
  if(reinterpret_cast<Extract>(scripts->extract)(params,data,offset,script,events,&move,&pip,&fight,&pov,&look,&text,&sneak)&&move==1&&pip==1&&fight==1&&pov==1&&!look&&!text&&!sneak){
   call(flightControlLocks);*result=0;return true;
  }
  *offset=start;
 }
 return originals[6](PASS);
}
// Conditions have no calling-script identity. Leave every condition evaluator native.
bool installVehicleCommands(bool driving,bool flight){
 const int count=flight?7:(driving?3:0);
 if(!count)return true;
 const char* names[]={"GetLeftStickX","GetRightTrigger","GetLeftTrigger","GetLeftStickY","IsButtonPressed","GetAngle","DisablePlayerControls"};
 const Exec handlers[]={steer,gas,stop,fore,buttons,flightAngle,flightLocks};
 Command* entries[7]{};
 DWORD protection[7]{};
 for(int i=0;i<count;++i){
  entries[i]=const_cast<Command*>(commands->byName(names[i]));
  if(!entries[i]||!entries[i]->execute)return false;
 }
 int prepared=0;
 while(prepared<count&&VirtualProtect(entries[prepared],sizeof(Command),PAGE_READWRITE,&protection[prepared]))++prepared;
 if(prepared==count){
  for(int i=0;i<count;++i){originals[i]=entries[i]->execute;entries[i]->execute=handlers[i];}
 }
 for(int i=prepared-1;i>=0;--i){DWORD previous;VirtualProtect(entries[i],sizeof(Command),protection[i],&previous);}
 return prepared==count;
}
#undef ARGS
#undef PASS
