#include "../reload_world_nif.hpp"
#include <fstream>
#include <iterator>
#include <iostream>
int main(int argc,char** argv){if(argc!=3)return 2;std::ifstream f(argv[1],std::ios::binary);std::vector<unsigned char>b((std::istreambuf_iterator<char>(f)),{});auto original=b;bool changed=false;if(!reload_world_nif::normalize(b,changed)||!changed)return 3;auto normalized=b;bool again=true;if(!reload_world_nif::normalize(b,again)||again||b!=normalized)return 4;std::ofstream out(argv[2],std::ios::binary);out.write(reinterpret_cast<char*>(b.data()),b.size());out.close();for(size_t cut=0;cut<original.size();cut++){auto truncated=original;truncated.resize(cut);auto before=truncated;bool c=false;if(reload_world_nif::normalize(truncated,c)||truncated!=before)return 5;}std::cout<<"normalized, idempotent, all truncations refused: "<<b.size()<<" bytes\n";return 0;}
