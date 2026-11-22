#pragma once
#include "wrist_frame.hpp"
#include "native_wrist_theme.hpp"
#include "../equipment_layout_view.hpp"
#include <filesystem>
#include <fstream>

namespace eyebot::device_art {
constexpr const char* names[]{"","general","eyebot","voice","pool","build","equipment"};
inline std::filesystem::path folder(){return "Data/Textures/FilthyCasual/Device";}
inline void glyph(HDC dc,wrist_frame::FilthyPage page,int x,int y,int size,COLORREF ink){
    if(page==wrist_frame::NoPage||unsigned(page)>=std::size(names))return;
    struct Icon {bool tried=false;std::array<uint32_t,64*64> pixels{};};
    static Icon icons[std::size(names)];
    auto& icon=icons[page];
    if(!icon.tried){
        icon.tried=true;
        std::ifstream file(folder()/(std::string(names[page])+".dds"),std::ios::binary);
        uint32_t header[32]{};
        if(file.read(reinterpret_cast<char*>(header),sizeof(header)) && header[0]==0x20534444 &&
           header[3]==64 && header[4]==64 && header[22]==32 && header[26]==0xff000000 &&
           file.read(reinterpret_cast<char*>(icon.pixels.data()),sizeof(icon.pixels))){
            for(auto& c:icon.pixels){
                unsigned a=c>>24;
                c=GetRValue(ink)*a/255<<16|GetGValue(ink)*a/255<<8|GetBValue(ink)*a/255;
            }
        }
    }
    BITMAPINFO info{};info.bmiHeader={sizeof(BITMAPINFOHEADER),64,-64,1,32,BI_RGB};
    StretchDIBits(dc,x,y,size,size,0,0,64,64,icon.pixels.data(),&info,DIB_RGB_COLORS,SRCCOPY);
}

inline bool controllers(const fc_equipment::Controllers& state,unsigned char* pixels,COLORREF ink){
    struct Atlas {
        std::string name;
        uint64_t masks[128]{};
        std::array<unsigned char,122*180*4> data{};
        bool ready=false;
    };
    static Atlas atlas[2];
    bool drawn=false;
    for(unsigned h=0;h<2;++h){
        auto& a=atlas[h];std::string model=state.model[h];
        if(model.empty()||model.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789-")!=std::string::npos)continue;
        if(a.name!=model){
            a.name=model;a.ready=false;
            std::ifstream f(folder()/(model+(h?"-right.lcd":"-left.lcd")),std::ios::binary);
            uint32_t magic{},count{};
            if(f.read(reinterpret_cast<char*>(&magic),4) && magic==0x31444346 &&
               f.read(reinterpret_cast<char*>(&count),4) && count>0 && count<128 &&
               f.read(reinterpret_cast<char*>(a.masks+1),count*8) &&
               f.read(reinterpret_cast<char*>(a.data.data()),a.data.size()))a.ready=true;
        }
        if(!a.ready)continue;
        drawn=true;
        const bool side=state.assigned&(1ull<<(7+h));
        const auto src=a.data.data()+(side?122*180*2:0);
        for(int y=0;y<180;++y)for(int x=0;x<122;++x){
            const unsigned off=2*(y*122+x),part=src[off+1];
            if(!part||part>=128)continue;
            const unsigned level=(a.masks[part]&state.assigned)?252:src[off];
            auto dst=pixels+4*((y+36)*256+x+4+h*126);
            dst[0]=uint8_t(level*GetBValue(ink)/255);dst[1]=uint8_t(level*GetGValue(ink)/255);
            dst[2]=uint8_t(level*GetRValue(ink)/255);dst[3]=255;
        }
    }
    return drawn;
}
}
