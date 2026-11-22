#pragma once
#include <array>
#include <algorithm>
#include <fstream>
// Bounded, opt-in diagnostics. No file IO in production or measurement scopes.
namespace vehicle_perf {
struct Bucket {std::array<double,4096> us{};unsigned count=0;double sum=0,max=0;};
inline Bucket buckets[6];
inline double frequency(){static double hz=[](){LARGE_INTEGER f;QueryPerformanceFrequency(&f);return double(f.QuadPart);}();return hz;}
struct Scope {
 int slot;LARGE_INTEGER begin{};
 Scope(int s,bool enabled):slot(enabled?s:-1){if(slot>=0)QueryPerformanceCounter(&begin);}
 ~Scope(){if(slot<0)return;LARGE_INTEGER end;QueryPerformanceCounter(&end);double us=double(end.QuadPart-begin.QuadPart)*1e6/frequency();auto& b=buckets[slot];b.us[b.count++%b.us.size()]=us;b.sum+=us;b.max=std::max(b.max,us);}
};
inline void write(const std::string& folder){
 if(folder.empty())return;std::ofstream out(folder+"\\vehicle-perf.json");
 const char* names[]={"mainLoop","beforePublication","finalizeOrigin","afterPublication","sampleSeatedAnimation","applySeatedAnimation"};out<<"{\"clock\":\"QueryPerformanceCounter\",\"unit\":\"microseconds\",\"components\":[";
 for(unsigned i=0;i<6;i++){auto& b=buckets[i];auto sorted=b.us;size_t n=std::min<size_t>(b.count,sorted.size());std::sort(sorted.begin(),sorted.begin()+n);if(i)out<<",";out<<"{\"name\":\""<<names[i]<<"\",\"count\":"<<b.count<<",\"mean\":"<<(b.count?b.sum/b.count:0)<<",\"p95Last4096\":"<<(n?sorted[size_t((n-1)*.95)]:0)<<",\"max\":"<<b.max<<"}";}out<<"]}";
}
}
