#pragma once
#include "controls_model.hpp"
#include <set>
#include <vector>
#include <cctype>
namespace controls_ini {
using namespace fc_controls;
inline const char* keys[]={"LeftTrigger","LeftGrip","LeftPrimary","LeftSecondary","LeftStickClick","RightTrigger","RightGrip","RightPrimary","RightSecondary","RightStickClick","MenuButton","MoveLeft","MoveRight","MoveBackward","MoveForward","TurnLeft","TurnRight","Crouch","Jump","LeftThumbRest","RightThumbRest","WeaponWheel","ConsumableWheel","ReloadMode","HeightSettings","WeaponTutorial"};
inline std::string trim(std::string s){auto a=s.find_first_not_of(" \t\r\n"),b=s.find_last_not_of(" \t\r\n");return a==s.npos?"":s.substr(a,b-a+1);}
inline std::string token(const std::string&s){std::string r;for(unsigned char c:s)if(std::isalnum(c))r+=char(std::tolower(c));return r;}
inline int source(const std::string&s){auto n=token(s);for(int i=0;i<sourceCount;i++)if(n==token(sourceNames[i]))return i;
 if(n=="leftprimary")return 2;if(n=="leftsecondary")return 3;if(n=="rightprimary")return 7;if(n=="rightsecondary")return 8;return -3;}
inline bool parse(const std::string&s,Binding&out){auto value=trim(s);if(value.empty()||token(value)=="default") {out={-1,-1,Level,600};return true;}if(token(value)=="off"||token(value)=="unbound"){out={-2,-1,Level,600};return true;}
 std::vector<std::string> parts;std::istringstream in(value);std::string v;while(std::getline(in,v,','))parts.push_back(trim(v));if(parts.empty()||parts.size()>3||value.back()==',')return false;
 auto plus=parts[0].find('+');Binding b;b.source=source(parts[0].substr(0,plus));b.modifier=plus==std::string::npos?-1:source(parts[0].substr(plus+1));
 if(parts.size()>1){auto g=token(parts[1]);int i=0;for(;i<5;i++)if(g==token(gestureNames[i]))break;if(i==5)return false;b.gesture=Gesture(i);}
 if(parts.size()>2){try{size_t n=0;b.delay=std::stoi(parts[2],&n);if(n!=parts[2].size())return false;}catch(...){return false;}}
 if(!valid(b)||b.source<0)return false;out=b;return true;}
struct Config {
 std::map<int,Binding> global;std::map<std::string,std::map<int,Binding>> modes;
 Binding editor{-2,-1,Level,600};bool openOnStart=false;std::string error;
 void apply(const std::string&id,Profile&p)const{for(auto&x:global)p.bindings[x.first]=x.second;auto m=modes.find(id);if(m!=modes.end())for(auto&x:m->second)p.bindings[x.first]=x.second;}
};
// Read only our sections; unrelated INI settings remain untouched. Reject the
// complete override transaction on any typo instead of installing partial input.
inline bool decode(const std::string&text,Config&out){if(text.size()>1024*1024)return false;Config c;std::istringstream in(text);std::string line,section;std::set<std::string> seen;
 while(std::getline(in,line)){auto comment=line.find(';');if(comment!=line.npos)line.resize(comment);line=trim(line);if(line.empty()||line[0]=='#')continue;
 if(line[0]=='['){if(line.back()!=']')return false;section=trim(line.substr(1,line.size()-2));std::transform(section.begin(),section.end(),section.begin(),[](unsigned char x){return char(std::tolower(x));});continue;}
 if(section!="bindings"&&section.rfind("bindings.",0)!=0)continue;
 auto equal=line.find('=');if(equal==line.npos)return false;auto key=token(trim(line.substr(0,equal))),value=trim(line.substr(equal+1));if(!seen.insert(section+":"+key).second)return false;
 if(section=="bindings"&&key=="opencontrolsonstart"){if(value!="0"&&value!="1")return false;c.openOnStart=value=="1";continue;}
 Binding b;if(!parse(value,b))return false;
 if(section=="bindings"&&key=="opencontrols"){c.editor=b;continue;}
 int row=0;for(;row<actionCount;row++)if(key==token(keys[row])||(row==10&&key=="leftmenu"))break;if(row==actionCount)return false;
 // Default means remove the override and keep the saved profile.
 if(b.source==-1)continue;
 if(section=="bindings")c.global[row]=b;
 else{auto id=section.substr(9);if(!validModeId(id))return false;c.modes[id][row]=b;}
 }out=c;return true;}
}
