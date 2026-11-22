#pragma once
#include "../build_adapter.hpp"
#include "../workshop-native-adapter.hpp"
#include "../pool-vr/adapter.hpp"
#include <array>
#include <limits>
#include <unordered_map>

namespace eyebot::equipment {
constexpr int ThumbSize=88;
using Thumbnail=std::array<unsigned char,ThumbSize*ThumbSize>;

// Sample the inventory form's own NIF once, away from the player scene. The
// device gets a compact depth-shaded image and never owns a game object.
inline Thumbnail thumbnail(const char* path){
    Thumbnail image{};
    auto node=pooladapter::loadOwnVisual(path);
    if(!node)return image;
    scene::retain(node);
    scene::update(node);
    std::vector<scene::Box> parts;
    scene::boxes(node,parts);
    scene::Vec ext;
    const auto center=scene::boxCenter(parts,&ext);
    const float span=std::max({ext.x,ext.y,ext.z});
    if(span>.001f && std::isfinite(span)){
        std::array<float,ThumbSize*ThumbSize> depth;
        depth.fill(-std::numeric_limits<float>::infinity());
        const float scale=(ThumbSize-4)/(span*1.12f);
        auto sample=[&](int x,int y,float z){
            if(x<0||x>=ThumbSize||y<0||y>=ThumbSize||!std::isfinite(z))return;
            auto index=y*ThumbSize+x;
            if(z<=depth[index])return;
            depth[index]=z;
            image[index]=static_cast<unsigned char>(std::clamp(163.f+z*140/span,70.f,255.f));
        };
        for(const auto& part:parts){
            auto data=scene::field<void*>(part.node,0xb8);
            const unsigned count=scene::field<unsigned short>(data,8);
            const auto vertices=scene::field<scene::Vec*>(data,0x20);
            const auto pose=scene::world(part.node);
            std::vector<scene::Vec> points(count);
            for(unsigned i=0;i<count;++i){
                auto p=pose.p+scene::rotate(pose,vertices[i])*pose.scale-center;
                if(ext.y>ext.x)std::swap(p.x,p.y);
                // Inventory meshes also ship lying on their side. Show their broad face.
                if(std::min(ext.x,ext.y)>ext.z)std::swap(p.y,p.z);
                points[i]={ThumbSize*.5f+(p.x*.96f+p.y*.24f)*scale,
                           ThumbSize*.5f-(p.z*.96f+p.y*.22f)*scale,-p.y};
                const auto v=points[i];
                if(!std::isfinite(v.x)||!std::isfinite(v.y)||std::abs(v.x)>ThumbSize*2||std::abs(v.y)>ThumbSize*2)continue;
                sample(int(v.x),int(v.y),v.z);
            }
            const auto triangle=[&](const unsigned short* ix){
                if(ix[0]>=count||ix[1]>=count||ix[2]>=count)return;
                const auto a=points[ix[0]], b=points[ix[1]], c=points[ix[2]];
                const float det=(b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x);
                if(!std::isfinite(det)||std::abs(det)<.01f)return;
                const int x0=int(std::clamp(std::min({a.x,b.x,c.x}),0.f,float(ThumbSize-1)));
                const int y0=int(std::clamp(std::min({a.y,b.y,c.y}),0.f,float(ThumbSize-1)));
                const int x1=int(std::clamp(std::max({a.x,b.x,c.x})+1,0.f,float(ThumbSize-1)));
                const int y1=int(std::clamp(std::max({a.y,b.y,c.y})+1,0.f,float(ThumbSize-1)));
                for(int y=y0;y<=y1;++y)for(int x=x0;x<=x1;++x){
                    float dx=x+.5f-a.x,dy=y+.5f-a.y;
                    float u=(dx*(c.y-a.y)-dy*(c.x-a.x))/det;
                    float v=((b.x-a.x)*dy-(b.y-a.y)*dx)/det;
                    if(u>=0&&v>=0&&u+v<=1)sample(x,y,a.z+u*(b.z-a.z)+v*(c.z-a.z));
                }
            };
            const auto table=scene::field<void**>(data,0);
            const auto rtti=reinterpret_cast<const char**(__thiscall*)(void*)>(table[2])(data);
            if(!rtti||!*rtti)continue;
            if(!strcmp(*rtti,"NiTriShapeData")){
                const auto indices=scene::field<unsigned short*>(data,0x48);
                const unsigned triangles=scene::field<unsigned short>(data,0x40);
                if(indices)for(unsigned j=0;j<triangles;++j)triangle(indices+3*j);
            }else if(!strcmp(*rtti,"NiTriStripsData")){
                const auto lengths=scene::field<unsigned short*>(data,0x48);
                auto indices=scene::field<unsigned short*>(data,0x4c);
                if(!lengths||!indices)continue;
                const unsigned strips=scene::field<unsigned short>(data,0x44);
                for(unsigned j=0;j<strips;++j){
                    for(unsigned k=2;k<lengths[j];++k)triangle(indices+k-2);
                    indices+=lengths[j];
                }
            }
        }
    }
    scene::release(node);
    return image;
}

class ItemPictures {
    Script* getModel=nullptr;
    std::unordered_map<std::string,Thumbnail> images;
public:
    const Thumbnail* get(uint32_t id,Scripts& scripts){
        auto form=workshop_native::lookup(id);
        if(!form)return nullptr;
        if(!getModel)getModel=scripts.compile("ref item\nbegin function {item}\nSetFunctionValue (GetModelPath item)\nend");
        Element result{};
        if(!getModel||!scripts.call(getModel,nullptr,nullptr,&result,1,form)||result.type!=3||!result.value.string)return nullptr;
        std::string path=result.value.string;
        build_adapter::freeGameString(result.value.string);
        if(path.empty())return nullptr;
        if(const auto it=images.find(path);it!=images.end())return &it->second;
        if(images.size()>=64)images.clear();
        return &images.emplace(path,thumbnail(path.c_str())).first->second;
    }
};
}
