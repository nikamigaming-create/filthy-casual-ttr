#pragma once

#include <d3d9.h>
#include <wrl/client.h>
#include "filthy_case_paint_data.hpp"

namespace eyebot::filthy_case_paint {

class Paint {
public:
    bool update(void* body) {
        // This is the case's existing diffuse texture, on the original
        // PPLighting material. Layout matches JIP LN's NiGeometry and NiTexture.
        auto property=body?scene::field<void*>(body,0xA8):nullptr;
        if(!property||scene::field<unsigned char>(property,0x1C)!=8){
            trace("property",property?scene::field<unsigned char>(property,0x1C):~0u);
            return false;
        }
        auto source=scene::field<void**>(property,0xAC);
        auto texture=source&&*source?scene::field<void*>(*source,0x24):nullptr;
        auto surface=texture?scene::field<IDirect3DBaseTexture9*>(texture,0x64):nullptr;
        if(!surface||surface->GetType()!=D3DRTYPE_TEXTURE){
            trace("surface",surface?surface->GetType():~0u);
            return false;
        }
        if(surface==installed.Get())return true;
        auto atlas=static_cast<IDirect3DTexture9*>(surface);
        D3DSURFACE_DESC desc{};
        if(FAILED(atlas->GetLevelDesc(0,&desc))||desc.Width!=1024||desc.Height!=1024||
           desc.Format!=D3DFMT_DXT1||atlas->GetLevelCount()<levels){
            trace("descriptor",desc.Width,desc.Height,unsigned(desc.Format),atlas->GetLevelCount());
            return false;
        }
        if(!paint(atlas))return false;
        installed=atlas;
        return true;
    }

    void reset(){installed.Reset();}

private:
    static void trace(const char* message,uint64_t a=0,unsigned b=0,unsigned c=0,unsigned d=0){
        char folder[MAX_PATH]{};
        if(GetEnvironmentVariableA("SEATED_TEST_DIR",folder,MAX_PATH)){
            auto path=std::string(folder)+"\\filthy-case-paint.txt";
            FILE* file=nullptr;
            if(!fopen_s(&file,path.c_str(),"a")){
                fprintf(file,"%s %llx %u %u %u\n",message,a,b,c,d);
                fclose(file);
            }
        }
    }
    static constexpr unsigned levels=5;
    Microsoft::WRL::ComPtr<IDirect3DBaseTexture9> installed;

    static bool paint(IDirect3DTexture9* atlas){
        D3DLOCKED_RECT rect[levels]{};
        unsigned locked=0;
        HRESULT hr=S_OK;
        while(locked<levels&&SUCCEEDED(hr=atlas->LockRect(locked,&rect[locked],nullptr,0)))++locked;
        if(locked!=levels)trace("lock",unsigned(hr),locked);
        bool painted=false;
        if(locked==levels){
            const auto texel=[&](const Block& block){
                return static_cast<unsigned char*>(rect[block.level].pBits)+
                       block.y*rect[block.level].Pitch+block.x*8;
            };
            uint64_t hash=14695981039346656037ull;
            for(const auto& block:blocks){
                const auto bytes=texel(block);
                for(unsigned i=0;i<8;++i)hash=(hash^bytes[i])*1099511628211ull;
            }
            // The Xbox case uses the same layout. Its label is already in
            // the local MO2 texture built by build_filthy_case_paint.py.
            constexpr uint64_t bakedXboxHash=0xaabd5dc46107cd36ull;
            painted=hash==stockHash||hash==paintedHash||hash==bakedXboxHash;
            trace(painted?"applied":"hash",hash);
            if(hash==stockHash){
                for(const auto& block:blocks){
                    auto bytes=texel(block);
                    uint64_t value;
                    std::memcpy(&value,bytes,sizeof(value));
                    value^=block.delta;
                    std::memcpy(bytes,&value,sizeof(value));
                }
            }
        }
        while(locked)atlas->UnlockRect(--locked);
        return painted;
    }
};

} // namespace eyebot::filthy_case_paint
