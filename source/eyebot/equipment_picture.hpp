#pragma once
#include "../equipment_layout_view.hpp"
#include "equipment_thumbnail.hpp"
#include "native_snapshot.hpp"
#include "native_wrist_theme.hpp"
#include <filesystem>
#include <fstream>

namespace eyebot::equipment {
using Picture=native_snapshot::Pixels;
inline void paintLayout(const fc_equipment::LayoutView& layout,Scripts& scripts,Picture& pixels){
    static ItemPictures pictures;
    pixels.fill(0);
    auto ink=native_theme::installed().colour();
    auto dot=[&](int x,int y,int level){
        if(unsigned(x)>=256||unsigned(y)>=256)return;
        auto p=pixels.data()+4*(256*y+x);
        p[0]=uint8_t(GetBValue(ink)*level/255);p[1]=uint8_t(GetGValue(ink)*level/255);
        p[2]=uint8_t(GetRValue(ink)*level/255);p[3]=255;
    };
    auto ring=[&](int x,int y,int r,int level,int width){
        for(int dy=-r;dy<=r;++dy)for(int dx=-r;dx<=r;++dx){
            int rr=dx*dx+dy*dy;
            if(rr<=r*r&&rr>=(r-width)*(r-width))dot(x+dx,y+dy,level);
        }
    };
    // The same clockwise slot order as the physical wheel, with its return cap at center.
    const bool belt=layout.kind==fc_equipment::Kind::Holsters;
    if(!belt){
        ring(128,128,89,88,2);
        const bool focused=layout.focus>=0 && unsigned(layout.focus)<layout.count;
        const auto form=focused?layout.slots[layout.focus].form:0;
        const auto image=form?pictures.get(form,scripts):nullptr;
        if(image){
            constexpr int size=104;
            for(int y=0;y<size;++y)for(int x=0;x<size;++x){
                const auto value=(*image)[(y*ThumbSize/size)*ThumbSize+x*ThumbSize/size];
                if(value)dot(128-size/2+x,128-size/2+y,value);
            }
        }else ring(128,128,15,145,2);
    }
    float minX=layout.slots[0].right,maxX=minX,minY=layout.slots[0].up,maxY=minY;
    for(unsigned i=1;i<layout.count;++i){
        const auto& s=layout.slots[i];minX=std::min(minX,s.right);maxX=std::max(maxX,s.right);
        minY=std::min(minY,s.up);maxY=std::max(maxY,s.up);
    }
    const float scale=belt?194.f/std::max({maxX-minX,maxY-minY,.14f}):89.f;
    constexpr unsigned digits[]{0,0x2492,0x73e7,0x73cf,0x5bc9,0x79cf,0x79ef,0x7249,0x7bef};
    for(unsigned i=0;i<layout.count;++i){
        const auto& slot=layout.slots[i];
        const int x=int(128+scale*(slot.right-(belt?(minX+maxX)*.5f:0)));
        const int y=int(128-scale*(slot.up-(belt?(minY+maxY)*.5f:0)));
        const bool selected=int(i)==layout.focus;
        const int r=belt?16:27;
        for(int dy=-r;dy<=r;++dy)for(int dx=-r;dx<=r;++dx)
            if(dx*dx+dy*dy<r*r)dot(x+dx,y+dy,16);
        ring(x,y,r,selected?255:slot.form?205:145,selected?4:2);
        const auto image=slot.form?pictures.get(slot.form,scripts):nullptr;
        bool drawn=false;
        if(image){
            int size=belt?26:41;
            for(int py=0;py<size;++py)for(int px=0;px<size;++px){
                auto shade=(*image)[(py*ThumbSize/size)*ThumbSize+px*ThumbSize/size];
                if(shade){dot(x-size/2+px,y-size/2+py,shade);drawn=true;}
            }
        }
        const int unit=drawn?2:3,top=drawn?y+r-3:y-unit*5/2;
        for(int by=0;by<5;++by)for(int bx=0;bx<3;++bx)
            if(digits[i+1]&(1u<<(14-by*3-bx)))
                for(int a=0;a<unit;++a)for(int b=0;b<unit;++b)dot(x-3*unit/2+bx*unit+a,top+by*unit+b,245);
    }
}
// Full Pip-Boy uses a stock image tile. Switch file identity on changes so
// the engine's texture cache sees the edit. The previous file is disposable.
class MenuTexture {
    unsigned serial=0;
    std::filesystem::path current;
public:
    void clear(){
        std::error_code error;
        if(!current.empty())std::filesystem::remove(current,error);
        current.clear();
    }
    std::string publish(const Picture& pixels){
        auto folder=std::filesystem::path("Data/Textures/RobCoEyebot");
        std::error_code error;
        std::filesystem::create_directories(folder,error);
        if(error)return {};
        const auto name="Equipment-"+std::to_string(GetCurrentProcessId())+"-"+std::to_string(++serial)+".dds";
        const auto file=folder/name;
        constexpr uint32_t header[32]{0x20534444,124,0x100f,256,256,1024,0,0,0,0,0,0,0,0,0,0,0,0,0,
            32,0x41,0,32,0xff0000,0xff00,0xff,0xff000000,0x1000,0,0,0,0};
        std::ofstream out(file,std::ios::binary);
        out.write(reinterpret_cast<const char*>(header),sizeof(header));
        out.write(reinterpret_cast<const char*>(pixels.data()),pixels.size());
        out.close();
        if(!out){std::filesystem::remove(file,error);return {};}
        clear();current=file;
        return "RobCoEyebot/"+name;
    }
};
}
