#pragma once
#include <vector>
#include <string>
#include <cstring>
#include <cstdint>
// Pure file transform. Never touches a live/shared material or the source NIF.
namespace reload_world_nif {
inline bool normalize(std::vector<unsigned char>& data, bool& changed, bool* missingUV=nullptr) {
 changed=false;if(missingUV)*missingUV=false;
 if(data.size()<128||data.size()>16*1024*1024)return false;
 auto result=data;size_t at=0;
 auto read=[&](unsigned n)->uint32_t{if(n>4||at>data.size()||n>data.size()-at)throw 1;uint32_t v=0;memcpy(&v,data.data()+at,n);at+=n;return v;};
 auto skip=[&](size_t n){if(at>data.size()||n>data.size()-at)throw 1;at+=n;};
 auto str=[&](){auto n=read(4);if(n>4096||n>data.size()-at)throw 1;std::string s(reinterpret_cast<const char*>(data.data()+at),n);at+=n;return s;};
 auto append32=[](std::vector<unsigned char>& bytes,uint32_t v){auto p=reinterpret_cast<const unsigned char*>(&v);bytes.insert(bytes.end(),p,p+4);};
 try {
  while(at<data.size()&&data[at++]!='\n'){};
  if(read(4)!=0x14020007||read(1)!=1||read(4)!=11)return false;
  auto count=read(4);if(!count||count>4096||read(4)!=34)return false;
  for(int i=0;i<3;i++)skip(read(1));auto nt=read(2);if(!nt||nt>256)return false;
  std::vector<std::string> types;for(unsigned i=0;i<nt;i++)types.push_back(str());
  std::vector<uint32_t> indices(count),sizes(count);for(auto& v:indices){v=read(2);if(v>=nt)return false;}
  auto sizeTable=at;for(auto& v:sizes)v=read(4);
  auto ns=read(4);read(4);if(ns>8192)return false;for(unsigned i=0;i<ns;i++)str();if(read(4)!=0)return false;
  result.resize(at);
  for(unsigned i=0;i<count;i++){
   const auto start=at;skip(sizes[i]);const auto end=at;
   std::vector<unsigned char> block(data.begin()+start,data.begin()+end);
   if(types[indices[i]]=="NiTriStripsData"||types[indices[i]]=="NiTriShapeData"){
    if(block.size()<15)return false;uint16_t vertices=0;memcpy(&vertices,block.data()+4,2);
    if(block[8]>1)return false;size_t flags=9+(block[8]?size_t(vertices)*12:0);if(flags+2>block.size())return false;
    if(missingUV&&(block[flags]&63)==0)*missingUV=true;
   }
   if(types[indices[i]]=="BSShaderPPLightingProperty"){
    if(block.size()<26)return false;uint32_t extras=0;memcpy(&extras,block.data()+4,4);
    if(extras>(block.size()-26)/4)return false;auto offset=size_t(22)+4*extras;
    uint32_t flags=0;memcpy(&flags,block.data()+offset,4);
    // JIP BSShaderProperty::kFlag2_1stPerson. Other flags/materials stay authored.
    if(flags&0x40){flags&=~0x40u;memcpy(block.data()+offset,&flags,4);changed=true;}
   }else if(types[indices[i]]=="BSShaderTextureSet"){
    at=start;auto textures=read(4);if(textures!=6)return false;block.clear();append32(block,textures);
    for(unsigned t=0;t<textures;t++){
     auto path=str();if(at>end)return false;
     if(path.size()>=5&&(path[0]=='D'||path[0]=='d')&&(path[1]=='A'||path[1]=='a')&&(path[2]=='T'||path[2]=='t')&&(path[3]=='A'||path[3]=='a')&&(path[4]=='\\'||path[4]=='/')){path.erase(0,5);changed=true;}
     append32(block,static_cast<uint32_t>(path.size()));block.insert(block.end(),path.begin(),path.end());
    }
    if(at!=end)return false;
   }
   uint32_t newSize=static_cast<uint32_t>(block.size());memcpy(result.data()+sizeTable+4*i,&newSize,4);
   result.insert(result.end(),block.begin(),block.end());at=end;
  }
  // Preserve and validate the root-link footer, rather than accepting truncation.
  auto footer=at;auto roots=read(4);if(roots>count)return false;for(unsigned i=0;i<roots;i++){auto root=read(4);if(root>=count)return false;}
  if(at!=data.size())return false;result.insert(result.end(),data.begin()+footer,data.end());
 }catch(...){changed=false;return false;}
 if(changed)data.swap(result);return true;
}
}
