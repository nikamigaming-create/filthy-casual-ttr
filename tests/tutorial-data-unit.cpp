#include "../tutorial_data.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
using namespace tutorial_data;
static void put(std::string& s,uint32_t v,unsigned n){for(unsigned i=0;i<n;i++)s+=char((v>>(8*i))&255);}
static std::string pack(const std::string& body){std::string s="TVPK";put(s,1,4);put(s,1,4);put(s,12,2);s+="handling.txt";put(s,34,4);put(s,unsigned(body.size()),4);return s+body;}
int main(int argc,char** argv){
 Database db;std::string error;
 const std::string valid="Weapon size -1 -2 -3 1 2 3\nWeapon rounds 1 2 3 0 1 0 0 0 0 0 0 0\n";
 auto p=pack(valid);std::istringstream input(p);assert(readPack(input,db,error));assert(db.size()==1&&db.at("weapon").hasRounds);
 for(size_t cut=0;cut<p.size();cut++){std::istringstream truncated(p.substr(0,cut));assert(!readPack(truncated,db,error));assert(db.empty());}
 auto corrupt=p;corrupt[26]=0;std::istringstream overlap(corrupt);assert(!readPack(overlap,db,error));
 assert(!readHandling("w size 3 2 1 -3 -2 -1\n",db,error));
 assert(!readHandling(valid+"Weapon rounds 1 2 3 0 1 0 0 0 0 0 0 0\n",db,error));
 assert(!readHandling("w size nan 0 0 1 1 1\n",db,error));
 assert(!readHandling("w rounds 1 2 3 0 1 0 0 0 0 0 0 0\n",db,error));
 if(argc>1){std::ifstream real(argv[1],std::ios::binary);if(!readPack(real,db,error)){std::cerr<<error<<"\n";return 2;}
  unsigned mags=0,handles=0,rounds=0,loaders=0;for(auto& e:db){mags+=e.second.mag.present;handles+=e.second.handle.present;rounds+=e.second.hasRounds;loaders+=e.second.loader.present;}
  std::cout<<"Installed metadata: "<<db.size()<<" weapons, "<<mags<<" magazines/cells, "<<handles<<" moving handles, "<<rounds<<" loading ports, "<<loaders<<" loaders\n";
  assert(db.at("weap10mmpistol").handle.node=="##Slide");assert(db.at("weaphuntingrifle").handle.node=="##HRBolt");
 }
 std::cout<<"PASS: bounded local TVPK reader; all truncations, overlap, invalid values and duplicate metadata rejected\n";
}
