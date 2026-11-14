#pragma once
#include <string>
#include <algorithm>
#include <cctype>
namespace empty_bottles {
// Matched to the user's FalloutNV.esm ALCH/MISC models. Do not pretend an
// unrelated junk model is a matching bottle. Unknown/addon shapes stay native.
inline unsigned baseForModel(std::string model){
 std::transform(model.begin(),model.end(),model.begin(),[](unsigned char c){return char(std::tolower(c));});std::replace(model.begin(),model.end(),'/', '\\');
 if(model=="clutter\\liquorbottles\\whiskeybottle01.nif")return 0x5B6CE;
 if(model=="clutter\\liquorbottles\\whiskeybottle02.nif")return 0x5B6CD;
 if(model=="clutter\\junk\\sodabottleclosed01.nif"||model=="clutter\\junk\\nv\\nvnukacolaquartz.nif"||model=="clutter\\junk\\nv\\nvnukacolavictory.nif"||model=="clutter\\questitems\\ms05quantumnukacola.nif")return 0x22103;
 if(model=="clutter\\junk\\ssbottle01.nif")return 0x103B1B;
 return 0;
}
}
