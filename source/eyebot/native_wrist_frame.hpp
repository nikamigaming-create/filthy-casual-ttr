#pragma once
#include "wrist_paint.hpp"
#include <array>

namespace eyebot::wrist_paint {

class NativeFrame {
public:
    bool copy(const wrist_frame::View& view,const unsigned char* photo,
              uint32_t* output,int width,int height) {
        if(width!=512 || height!=384) return false;
        if(!view.generation || !initialize()){
            std::fill_n(output,width*height,0xff000000u);
            return false;
        }
        if(generation!=view.generation){
            page(dc,view,photo);
            GdiFlush();
            // TTR and the DIB use the same channel order. Set the alpha for display.
            for(int y=0;y<height;++y){
                const auto row=pixels+(y*Height/height)*Width;
                for(int x=0;x<width;++x){
                    const auto rgb=row[x];
                    rgba[y*width+x]=0xff000000u|rgb;
                }
            }
            generation=view.generation;
        }
        std::memcpy(output,rgba.data(),sizeof(rgba));
        return true;
    }
    ~NativeFrame(){
        if(bitmap){SelectObject(dc,previous);DeleteObject(bitmap);}
        if(dc)DeleteDC(dc);
    }
private:
    bool initialize(){
        if(bitmap)return true;
        if(!dc)dc=CreateCompatibleDC(nullptr);
        if(!dc)return false;
        BITMAPINFO info{};
        info.bmiHeader={sizeof(BITMAPINFOHEADER),Width,-Height,1,32,BI_RGB};
        void* bits=nullptr;
        bitmap=CreateDIBSection(dc,&info,DIB_RGB_COLORS,&bits,nullptr,0);
        if(!bitmap)return false;
        previous=SelectObject(dc,bitmap);
        pixels=static_cast<const uint32_t*>(bits);
        return true;
    }
    HDC dc{};
    HBITMAP bitmap{};
    HGDIOBJ previous{};
    const uint32_t* pixels{};
    uint32_t generation{};
    std::array<uint32_t,512*384> rgba{};
};
}
