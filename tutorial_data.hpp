#pragma once
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <istream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
// Read only the installed TTR1.1 TVPK metadata. Never extract/bundle its meshes
// or data. Coordinates below are measured in the native weapon model frame.
namespace tutorial_data {
struct Vec {float x=0,y=0,z=0;};
struct Part {bool present=false;std::string node;Vec face{},axis{};float spread=0;std::vector<float> values;};
struct Weapon {
 bool hasSize=false,hasRounds=false,hasWell=false;Vec lo{},hi{},rounds{},roundAxis{},wellOut{};
 float travel=0;Part mag,handle,loader,take,home,charger;
};
using Database=std::unordered_map<std::string,Weapon>;
inline std::string key(std::string s){for(auto& c:s)c=char(std::tolower(static_cast<unsigned char>(c)));return s;}
inline bool readHandling(const std::string& text,Database& out,std::string& error){
 out.clear();Database pending;std::istringstream lines(text);std::string line;unsigned rows=0;
 auto fail=[&](const char* why){error=why;return false;};
 while(std::getline(lines,line)){
  if(++rows>30000||line.size()>4096)return fail("handling bounds exceeded");
  std::istringstream row(line);std::string id,type,node;if(!(row>>id)||id[0]==';')continue;
  if(!(row>>type)||id.size()>200)return fail("invalid handling key");
  const bool part=type=="mag"||type=="handle"||type=="loader"||type=="take"||type=="home"||type=="charger";
  const bool known=part||type=="size"||type=="rounds"||type=="well"||type=="hold";
  if(!known)continue;
  if(part&&(!(row>>node)||node.size()>256))return fail("invalid part name");
  std::vector<float> v;std::string token;
  while(row>>token){
   char* end=nullptr;float f=std::strtof(token.c_str(),&end);
   if(end!=token.c_str()+token.size()||!std::isfinite(f)||std::abs(f)>100000||v.size()>=64)return fail("invalid handling value");
   v.push_back(f);
  }
  const size_t expected=type=="size"?6:type=="mag"||type=="handle"?26:type=="loader"?7:type=="take"||type=="home"?3:type=="charger"?4:type=="hold"?8:12;
  if(v.size()!=expected)return fail("unsupported handling row layout");
  auto& w=pending[key(id)];if(pending.size()>10000)return fail("too many weapons");
  auto at=[&](size_t i){return Vec{v[i],v[i+1],v[i+2]};};
  if(type=="size"){
   if(w.hasSize)return fail("duplicate weapon bounds");w.hasSize=true;w.lo=at(0);w.hi=at(3);
   if(w.lo.x>w.hi.x||w.lo.y>w.hi.y||w.lo.z>w.hi.z)return fail("inverted weapon bounds");
  }else if(type=="rounds"){
   if(w.hasRounds)return fail("duplicate loading point");w.hasRounds=true;w.rounds=at(0);w.roundAxis=at(3);
  }else if(type=="well"){
   if(w.hasWell)return fail("duplicate well");w.hasWell=true;w.wellOut=at(0);w.travel=v[3];
  }else if(part){
   auto& p=type=="mag"?w.mag:type=="handle"?w.handle:type=="loader"?w.loader:type=="take"?w.take:type=="home"?w.home:w.charger;
   if(p.present)return fail("duplicate part");p.present=true;p.node=node;p.values=v;
   // Charger begins with travel, not a position. Preserve its exact schema.
   if(type=="charger"){p.axis=at(1);}else {p.face=at(0);if(v.size()>=6)p.axis=at(3);}
   if(type=="mag"||type=="handle")p.spread=v[6];
  }
 }
 if(pending.empty())return fail("empty handling metadata");
 for(auto& entry:pending)if(!entry.second.hasSize)return fail("weapon missing bounds");
 out=std::move(pending);error.clear();return true;
}
inline bool readPack(std::istream& file,Database& out,std::string& error){
 out.clear();auto fail=[&](const char* why){error=why;return false;};
 file.seekg(0,std::ios::end);auto end=file.tellg();
 if(end<12||end>128*1024*1024)return fail("invalid TVPK length");
 const auto length=uint64_t(end);file.seekg(0);
 char magic[4];file.read(magic,4);if(std::string(magic,4)!="TVPK")return fail("invalid TVPK signature");
 auto integer=[&](unsigned bytes,uint32_t& value){value=0;for(unsigned i=0;i<bytes;i++){char c;if(!file.get(c))return false;value|=uint32_t(static_cast<unsigned char>(c))<<(8*i);}return true;};
 uint32_t version=0,count=0;if(!integer(4,version)||version!=1||!integer(4,count)||count==0||count>128)return fail("unsupported TVPK directory");
 struct Entry {uint32_t offset,size;};std::vector<Entry> entries;Entry handling{};bool found=false;
 for(uint32_t i=0;i<count;i++){
  uint32_t n=0;if(!integer(2,n)||!n||n>256)return fail("invalid TVPK entry name");
  std::string name(n,'\0');if(!file.read(&name[0],n))return fail("truncated TVPK name");
  Entry e{};if(!integer(4,e.offset)||!integer(4,e.size)||uint64_t(e.offset)+e.size>length)return fail("invalid TVPK entry range");
  entries.push_back(e);
  if(name=="handling.txt"){if(found||!e.size||e.size>4*1024*1024)return fail("invalid handling entry");handling=e;found=true;}
 }
 auto directoryEnd=uint64_t(file.tellg());std::sort(entries.begin(),entries.end(),[](Entry a,Entry b){return a.offset<b.offset;});
 uint64_t previous=directoryEnd;for(auto e:entries){if(e.offset<previous)return fail("overlapping TVPK entry");previous=uint64_t(e.offset)+e.size;}
 if(!found)return fail("handling metadata not installed");
 file.seekg(handling.offset);std::string text(handling.size,'\0');if(!file.read(&text[0],handling.size))return fail("truncated handling metadata");
 return readHandling(text,out,error);
}
}
