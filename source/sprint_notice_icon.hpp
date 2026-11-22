#pragma once
// Read one installed-game icon into memory. No game asset is distributed or cached on disk.
#include <fstream>
#include <filesystem>
#include <vector>
#include <string>
#include <cstdint>
#define MINIZ_NO_ARCHIVE_APIS
#define MINIZ_NO_STDIO
#include "third_party/miniz/miniz_tinfl.h"
namespace sprint_icon {
inline std::vector<unsigned char> rgba;inline bool attempted=false;
inline uint32_t u32(const unsigned char* p){uint32_t n;memcpy(&n,p,4);return n;}
inline bool read(std::ifstream& f,void* p,size_t n){return bool(f.read((char*)p,n));}
inline bool load(){
 if(attempted)return !rgba.empty();attempted=true;
 wchar_t exe[MAX_PATH]{};GetModuleFileNameW(nullptr,exe,MAX_PATH);auto data=std::filesystem::path(exe).parent_path()/L"Data";
 const std::string folder="textures\\interface\\icons\\pipboyimages\\perks",name="perk_travel_light.dds";
 std::vector<unsigned char> dds;
 std::ifstream loose(data/std::filesystem::path(folder)/name,std::ios::binary|std::ios::ate);
 if(loose){auto n=loose.tellg();if(n>128&&n<2*1024*1024){dds.resize(size_t(n));loose.seekg(0);if(!read(loose,dds.data(),dds.size()))return false;}}
 if(dds.empty()){
  std::ifstream f(data/L"Fallout - Textures2.bsa",std::ios::binary);unsigned char h[36];if(!read(f,h,36)||memcmp(h,"BSA\0",4)||u32(h+4)!=104)return false;
  uint32_t offset=u32(h+8),flags=u32(h+12),nf=u32(h+16),nn=u32(h+20),namesSize=u32(h+28);
  if(nf>100000||nn>1000000||namesSize>32*1024*1024||(flags&3)!=3)return false;
  struct File{std::string folder;uint32_t size,offset;};std::vector<File> files;std::vector<uint32_t> counts;
  f.seekg(offset);for(uint32_t i=0;i<nf;i++){unsigned char r[16];if(!read(f,r,16))return false;counts.push_back(u32(r+8));}
  for(auto count:counts){unsigned char n;if(!read(f,&n,1)||!n)return false;std::string dir(n,'\0');if(!read(f,dir.data(),n))return false;if(dir.back()=='\0')dir.pop_back();if(count>nn||files.size()+count>nn)return false;
   for(uint32_t j=0;j<count;j++){unsigned char r[16];if(!read(f,r,16))return false;files.push_back({dir,u32(r+8),u32(r+12)});}}
  if(files.size()!=nn)return false;std::vector<char> names(namesSize);if(!read(f,names.data(),names.size()))return false;size_t at=0;
  for(auto& file:files){size_t end=at;while(end<names.size()&&names[end])++end;if(end==names.size())return false;std::string current(names.data()+at,end-at);at=end+1;
   if(_stricmp(file.folder.c_str(),folder.c_str())||_stricmp(current.c_str(),name.c_str()))continue;
   uint32_t n=file.size&0x3fffffff;if(n<4||n>2*1024*1024)return false;std::vector<unsigned char> stored(n);f.seekg(file.offset);if(!read(f,stored.data(),n))return false;
   size_t start=0;if(flags&0x100){start=size_t(stored[0])+1;if(start>=stored.size())return false;}
   bool compressed=((flags&4)!=0)!=((file.size&0x40000000)!=0);
   if(compressed){if(start+4>=n)return false;uint32_t out=u32(stored.data()+start);if(out<128||out>2*1024*1024)return false;dds.resize(out);if(tinfl_decompress_mem_to_mem(dds.data(),out,stored.data()+start+4,n-start-4,TINFL_FLAG_PARSE_ZLIB_HEADER)!=out)return false;}
   else dds.assign(stored.begin()+start,stored.end());break;
  }
 }
 // Only the bounded BC3 layout used by this icon; fail closed for other formats.
 if(dds.size()<128||memcmp(dds.data(),"DDS ",4)||u32(dds.data()+12)!=256||u32(dds.data()+16)!=256||memcmp(dds.data()+84,"DXT5",4)||dds.size()<128+65536)return false;
 rgba.resize(256*256*4);
 for(int by=0;by<64;by++)for(int bx=0;bx<64;bx++){
  auto b=dds.data()+128+(by*64+bx)*16;int al[8]={b[0],b[1]};
  if(al[0]>al[1])for(int i=2;i<8;i++)al[i]=((8-i)*al[0]+(i-1)*al[1])/7;
  else {for(int i=2;i<6;i++)al[i]=((6-i)*al[0]+(i-1)*al[1])/5;al[6]=0;al[7]=255;}
  uint64_t ai=0;for(int i=0;i<6;i++)ai|=uint64_t(b[2+i])<<(8*i);
  int col[4][3]{};for(int i=0;i<2;i++){unsigned c=b[8+i*2]|(unsigned(b[9+i*2])<<8);col[i][0]=((c>>11)&31)*255/31;col[i][1]=((c>>5)&63)*255/63;col[i][2]=(c&31)*255/31;}
  for(int c=0;c<3;c++){col[2][c]=(2*col[0][c]+col[1][c])/3;col[3][c]=(col[0][c]+2*col[1][c])/3;}
  uint32_t ci=u32(b+12);for(int i=0;i<16;i++){auto out=&rgba[((by*4+i/4)*256+bx*4+i%4)*4];int c=(ci>>(i*2))&3;out[0]=uint8_t(col[c][2]);out[1]=uint8_t(col[c][1]);out[2]=uint8_t(col[c][0]);out[3]=uint8_t(al[(ai>>(i*3))&7]);}
 }
 return true;
}
inline void draw(void* destination,int width,int height){
 if(!load())return;auto p=(unsigned char*)destination;
 for(int y=0;y<256&&y+112<height;y++)for(int x=0;x<256&&x+160<width;x++){
  auto in=&rgba[(y*256+x)*4],out=&p[((y+112)*width+x+160)*4];unsigned a=in[3];unsigned lum=std::max(in[0],std::max(in[1],in[2]));a=a*lum/255;
  out[0]=139;out[1]=245;out[2]=173;out[3]=uint8_t(a);
 }
}
}
