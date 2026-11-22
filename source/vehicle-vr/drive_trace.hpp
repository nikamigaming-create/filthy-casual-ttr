#pragma once
#include "../scene_adapter.hpp"
#include "motion_adapter.hpp"
#include <fstream>
// Private bounded trace: compare reference motion with the rendered chassis in
// the same publication. Separate snapshots cannot establish velocity or jitter.
namespace drive_trace {
inline void record(const std::string& directory, Form* car, void* carrier,
                   unsigned publication, float commandedSpeed, float gameDelta) {
 if(directory.empty()||!car||!carrier)return;
 static std::ofstream out(directory+"\\drive-publication.jsonl",std::ios::trunc);
 static unsigned count=0;if(!out||count++>=16000)return;
 auto root=scene::root(car);if(!root)return;
 auto actor=scene::field<scene::Vec>(car,0x30);
 auto r=scene::world(root),c=scene::world(carrier);
 LARGE_INTEGER time;QueryPerformanceCounter(&time);
 out.precision(10);
 out<<"{\"pid\":"<<GetCurrentProcessId()<<",\"car\":"<<car->id<<",\"publication\":"<<publication
    <<",\"qpc\":"<<time.QuadPart<<",\"dt\":"<<gameDelta
    <<",\"commandedSpeed\":"<<commandedSpeed<<",\"scale\":"<<r.scale
    <<",\"motionCalls\":"<<vehicle_motion::calls.load()<<",\"motionDt\":"<<vehicle_motion::lastDelta.load()
    <<",\"motionBefore\":"<<vehicle_motion::lastBefore.load()<<",\"motionAfter\":"<<vehicle_motion::lastAfter.load()
    <<",\"actor\":["<<actor.x<<","<<actor.y<<","<<actor.z
    <<"],\"root\":["<<r.p.x<<","<<r.p.y<<","<<r.p.z
    <<"],\"carrier\":["<<c.p.x<<","<<c.p.y<<","<<c.p.z<<"]}\n";
 if(count%60==0)out.flush();
}
}
