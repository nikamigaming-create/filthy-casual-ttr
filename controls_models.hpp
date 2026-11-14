#pragma once
// Adapted from MIT MGS5VR controls_panel_renderer.cpp. License shipped with assets.
#include <d3dcompiler.h>
#include <wincodec.h>
#include <wrl/client.h>
#include <string_view>
#include "controls_runtime.hpp"
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib,"windowscodecs.lib")
#pragma comment(lib,"ole32.lib")
namespace controls_models {
using Microsoft::WRL::ComPtr;
constexpr UINT panelWidth=1400,panelHeight=1000;
inline std::filesystem::path assets(){wchar_t path[MAX_PATH]{};auto m=GetModuleHandleW(L"SeatedInput.dll");GetModuleFileNameW(m,path,MAX_PATH);return std::filesystem::path(path).parent_path()/L"Controls";}
struct ModelVertex {float position[3],normal[3],uv[2];};
struct ModelPart {ComPtr<ID3D11Buffer> vertices,indices;UINT count{};uint64_t inputs{};};
struct Model {std::vector<ModelPart> parts;ComPtr<ID3D11ShaderResourceView> texture;};
bool textureFile(ID3D11Device* device,const std::filesystem::path& path,ComPtr<ID3D11ShaderResourceView>& output){
    const auto initialized=CoInitializeEx(nullptr,COINIT_MULTITHREADED);
    struct Apartment {HRESULT result;~Apartment(){if(SUCCEEDED(result))CoUninitialize();}} apartment{initialized};
    ComPtr<IWICImagingFactory> factory;ComPtr<IWICBitmapDecoder> decoder;ComPtr<IWICBitmapFrameDecode> frame;ComPtr<IWICFormatConverter> converter;
    if(FAILED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&factory)))
        ||FAILED(factory->CreateDecoderFromFilename(path.c_str(),nullptr,GENERIC_READ,WICDecodeMetadataCacheOnLoad,&decoder))
        ||FAILED(decoder->GetFrame(0,&frame))||FAILED(factory->CreateFormatConverter(&converter))
        ||FAILED(converter->Initialize(frame.Get(),GUID_WICPixelFormat32bppRGBA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom)))return false;
    UINT width{},height{};if(FAILED(frame->GetSize(&width,&height))||!width||!height||width>4096||height>4096)return false;
    std::vector<uint32_t> pixels(size_t(width)*height);
    if(FAILED(converter->CopyPixels(nullptr,width*4,static_cast<UINT>(pixels.size()*4),reinterpret_cast<BYTE*>(pixels.data()))))return false;
    D3D11_TEXTURE2D_DESC desc{};desc.Width=width;desc.Height=height;desc.MipLevels=desc.ArraySize=desc.SampleDesc.Count=1;
    desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;desc.Usage=D3D11_USAGE_IMMUTABLE;desc.BindFlags=D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA data{pixels.data(),width*4,0};ComPtr<ID3D11Texture2D> texture;
    return SUCCEEDED(device->CreateTexture2D(&desc,&data,&texture))&&SUCCEEDED(device->CreateShaderResourceView(texture.Get(),nullptr,&output));
}
bool modelFile(ID3D11Device* device,const std::filesystem::path& path,Model& model){
    std::ifstream file(path,std::ios::binary);char magic[4]{};uint32_t version{},count{};
    file.read(magic,4);file.read(reinterpret_cast<char*>(&version),4);file.read(reinterpret_cast<char*>(&count),4);
    if(!file||std::string_view(magic,4)!="M5CT"||(version!=1&&version!=2)||count>128)return false;
    for(uint32_t p=0;p<count;++p){
        ModelPart part;uint32_t vertices{},indices{};
        file.read(reinterpret_cast<char*>(&part.inputs),version==2?8:4);file.read(reinterpret_cast<char*>(&vertices),4);file.read(reinterpret_cast<char*>(&indices),4);
        if(!file||!vertices||vertices>250000||!indices||indices>250000)return false;
        std::vector<ModelVertex> vertex(vertices);std::vector<uint32_t> index(indices);
        file.read(reinterpret_cast<char*>(vertex.data()),static_cast<std::streamsize>(vertex.size()*sizeof(ModelVertex)));
        file.read(reinterpret_cast<char*>(index.data()),static_cast<std::streamsize>(index.size()*sizeof(uint32_t)));
        if(!file||std::any_of(index.begin(),index.end(),[&](uint32_t n){return n>=vertices;}))return false;
        D3D11_BUFFER_DESC desc{};desc.ByteWidth=static_cast<UINT>(vertex.size()*sizeof(ModelVertex));desc.Usage=D3D11_USAGE_IMMUTABLE;desc.BindFlags=D3D11_BIND_VERTEX_BUFFER;
        D3D11_SUBRESOURCE_DATA data{vertex.data(),0,0};if(FAILED(device->CreateBuffer(&desc,&data,&part.vertices)))return false;
        desc.ByteWidth=indices*4;desc.BindFlags=D3D11_BIND_INDEX_BUFFER;data.pSysMem=index.data();
        if(FAILED(device->CreateBuffer(&desc,&data,&part.indices)))return false;
        part.count=indices;model.parts.push_back(std::move(part));
    }
    auto texture=path;texture.replace_extension(L".png");return textureFile(device,texture,model.texture);
}
constexpr char shader[]=R"(
Texture2D image:register(t0);SamplerState linearClamp:register(s0);
cbuffer Params:register(b0){float4 pose;float4 tint;};
struct Vertex{float3 p:POSITION;float3 n:NORMAL;float2 uv:TEXCOORD;};
struct Pixel{float4 p:SV_Position;float3 n:NORMAL;float2 uv:TEXCOORD;};
Pixel modelVs(Vertex v){Pixel o;float3 p=v.p,n=v.n;
 float3 rotated=float3(p.x*pose.x+p.z*pose.y,p.y,-p.x*pose.y+p.z*pose.x);
 rotated=float3(rotated.x,rotated.y*pose.z-rotated.z*pose.w,rotated.y*pose.w+rotated.z*pose.z);
 o.p=float4(rotated.x*11.2658,rotated.y*12,.5-rotated.z*3,1);
 float3 normal=float3(n.x*pose.x+n.z*pose.y,n.y,-n.x*pose.y+n.z*pose.x);
 o.n=float3(normal.x,normal.y*pose.z-normal.z*pose.w,normal.y*pose.w+normal.z*pose.z);o.uv=v.uv;return o;}
float4 modelPs(Pixel v):SV_Target{float3 base=image.Sample(linearClamp,v.uv).rgb;
 base=max(base,float3(.09,.09,.09)); float light=.80+.40*saturate(dot(normalize(v.n),normalize(float3(-.3,.6,1))));
 return float4(lerp(base*light,tint.rgb,tint.a),1);}
Pixel panelVs(uint id:SV_VertexID){Pixel o;o.uv=float2((id<<1)&2,id&2);o.p=float4(o.uv*float2(2,-2)+float2(-1,1),0,1);o.n=0;return o;}
float4 panelPs(Pixel v):SV_Target{return image.Sample(linearClamp,v.uv);}
)";
struct Renderer {
    ComPtr<ID3D11Texture2D> color,output,depth,background;
    ComPtr<ID3D11RenderTargetView> target;
    ComPtr<ID3D11DepthStencilView> depthView;
    ComPtr<ID3D11ShaderResourceView> backgroundView;
    ComPtr<ID3D11VertexShader> modelVs,panelVs;
    ComPtr<ID3D11PixelShader> modelPs,panelPs;
    ComPtr<ID3D11InputLayout> layout;
    ComPtr<ID3D11Buffer> constants;
    ComPtr<ID3D11SamplerState> sampler;
    ComPtr<ID3D11DepthStencilState> depthOn,depthOff;
    ComPtr<ID3D11RasterizerState> raster;
    std::array<Model,2> models;
    ComPtr<ID3D11DeviceContext> deferred;
    bool ready{},failed{};std::array<std::string,2> loadedAssets{};
    UINT samples{1};
    std::array<float,2> yaw{},pitch{};
    bool initialize(ID3D11Device* device,const std::filesystem::path& game){
        const auto compile=[&](const char* entry,const char* profile,ComPtr<ID3DBlob>& result){ComPtr<ID3DBlob> errors;
            return SUCCEEDED(D3DCompile(shader,sizeof(shader)-1,nullptr,nullptr,nullptr,entry,profile,D3DCOMPILE_ENABLE_STRICTNESS,0,&result,&errors));};
        ComPtr<ID3DBlob> code;
        if(!compile("modelVs","vs_5_0",code)||FAILED(device->CreateVertexShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,&modelVs)))return false;
        const D3D11_INPUT_ELEMENT_DESC input[]{{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},
            {"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,12,D3D11_INPUT_PER_VERTEX_DATA,0},{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,24,D3D11_INPUT_PER_VERTEX_DATA,0}};
        if(FAILED(device->CreateInputLayout(input,3,code->GetBufferPointer(),code->GetBufferSize(),&layout)))return false;
        if(!compile("modelPs","ps_5_0",code)||FAILED(device->CreatePixelShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,&modelPs))
            ||!compile("panelVs","vs_5_0",code)||FAILED(device->CreateVertexShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,&panelVs))
            ||!compile("panelPs","ps_5_0",code)||FAILED(device->CreatePixelShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,&panelPs)))return false;
        D3D11_TEXTURE2D_DESC texture{};texture.Width=panelWidth;texture.Height=panelHeight;texture.MipLevels=texture.ArraySize=texture.SampleDesc.Count=1;
        texture.Format=DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;texture.BindFlags=D3D11_BIND_SHADER_RESOURCE;
        if(FAILED(device->CreateTexture2D(&texture,nullptr,&background))||FAILED(device->CreateShaderResourceView(background.Get(),nullptr,&backgroundView))
            ||FAILED(device->CreateTexture2D(&texture,nullptr,&output)))return false;
        UINT quality{};if(SUCCEEDED(device->CheckMultisampleQualityLevels(texture.Format,4,&quality))&&quality)samples=4;
        texture.SampleDesc.Count=samples;texture.BindFlags=D3D11_BIND_RENDER_TARGET;
        if(FAILED(device->CreateTexture2D(&texture,nullptr,&color))||FAILED(device->CreateRenderTargetView(color.Get(),nullptr,&target)))return false;
        texture.Format=DXGI_FORMAT_D32_FLOAT;texture.BindFlags=D3D11_BIND_DEPTH_STENCIL;
        if(FAILED(device->CreateTexture2D(&texture,nullptr,&depth))||FAILED(device->CreateDepthStencilView(depth.Get(),nullptr,&depthView)))return false;
        D3D11_BUFFER_DESC buffer{};buffer.ByteWidth=32;buffer.BindFlags=D3D11_BIND_CONSTANT_BUFFER;
        if(FAILED(device->CreateBuffer(&buffer,nullptr,&constants)))return false;
        D3D11_SAMPLER_DESC sample{};sample.Filter=D3D11_FILTER_MIN_MAG_MIP_LINEAR;sample.AddressU=sample.AddressV=sample.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;sample.MaxLOD=D3D11_FLOAT32_MAX;
        if(FAILED(device->CreateSamplerState(&sample,&sampler)))return false;
        D3D11_DEPTH_STENCIL_DESC z{};z.DepthEnable=TRUE;z.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ALL;z.DepthFunc=D3D11_COMPARISON_LESS;
        if(FAILED(device->CreateDepthStencilState(&z,&depthOn)))return false;z.DepthEnable=FALSE;
        if(FAILED(device->CreateDepthStencilState(&z,&depthOff)))return false;
        D3D11_RASTERIZER_DESC rs{};rs.FillMode=D3D11_FILL_SOLID;rs.CullMode=D3D11_CULL_NONE;rs.DepthClipEnable=TRUE;rs.MultisampleEnable=TRUE;
        if(FAILED(device->CreateRasterizerState(&rs,&raster)))return false;
        (void)game;
        if(FAILED(device->CreateDeferredContext(0,&deferred)))return false;ready=true;return true;
    }
};
inline Renderer renderer;
inline uint64_t sourceMask(int source){if(source<0||source>=70)return 0;static constexpr int index[]={9,7,2,3,5,10,8,0,1,6,4,11,12,13,14,15,16,17,18,19,20,25,26,23,24,21,22,27,28,29,30,31,30,31,30,30,30,30,31,31,31,31,7,8,30,31,32,33,34,35,36,37,38,39,7,8,32,33,34,35,36,37,38,39,4,29,9,10,7,8};return 1ull<<index[source];}
inline uint64_t modelMask(const fc_controls::Frame&f,bool touchOnly=false){uint64_t mask=0;for(int i=0;i<fc_controls::sourceCount;i++)if(f[i].active&&f[i].value>.15f&&controller_profiles::touch(i)==touchOnly)mask|=sourceMask(i);return mask;}
inline void cleanup(){renderer=Renderer{};}
inline ID3D11Texture2D* draw(ID3D11Device*device,ID3D11DeviceContext*immediate,const controls_runtime::View&f,const std::vector<unsigned char>&pixels){
 auto&p=renderer;if(p.failed)return nullptr;if(!p.ready&&!p.initialize(device,assets())){p.failed=true;return nullptr;}
 auto* context=p.deferred.Get();context->UpdateSubresource(p.background.Get(),0,nullptr,pixels.data(),panelWidth*4,0);
    context->ClearDepthStencilView(p.depthView.Get(),D3D11_CLEAR_DEPTH,1,0);
    auto* target=p.target.Get();context->OMSetRenderTargets(1,&target,p.depthView.Get());context->OMSetBlendState(nullptr,nullptr,~0u);
    context->RSSetState(p.raster.Get());D3D11_VIEWPORT viewport{0,0,static_cast<float>(panelWidth),static_cast<float>(panelHeight),0,1};context->RSSetViewports(1,&viewport);
    context->OMSetDepthStencilState(p.depthOff.Get(),0);context->IASetInputLayout(nullptr);context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->VSSetShader(p.panelVs.Get(),nullptr,0);context->PSSetShader(p.panelPs.Get(),nullptr,0);
    auto* sampler=p.sampler.Get();context->PSSetSamplers(0,1,&sampler);auto* image=p.backgroundView.Get();context->PSSetShaderResources(0,1,&image);context->Draw(3,0);
    context->OMSetDepthStencilState(p.depthOn.Get(),0);context->IASetInputLayout(p.layout.Get());context->VSSetShader(p.modelVs.Get(),nullptr,0);context->PSSetShader(p.modelPs.Get(),nullptr,0);
    auto* constants=p.constants.Get();context->VSSetConstantBuffers(0,1,&constants);context->PSSetConstantBuffers(0,1,&constants);
    const auto pressedMask=modelMask(f.physical),touchMask=modelMask(f.physical,true),active=pressedMask|touchMask;uint64_t assigned=0;
    if(f.selected<fc_controls::actionCount){auto b=f.phase==3?f.proposal:f.profile.bindings[f.selected];assigned=sourceMask(b.source)|sourceMask(b.modifier);}
    for(size_t hand=0;hand<2;++hand){
        auto asset=f.deviceAssets[hand];if(p.loadedAssets[hand]!=asset){p.models[hand]=Model{};p.loadedAssets[hand]=asset;if(!asset.empty()){auto path=assets()/asset/(hand?L"right.mesh":L"left.mesh");if(!modelFile(device,path,p.models[hand]))p.models[hand]=Model{};}}
        viewport={hand?245.f:25.f,245,255,405,0,1};context->RSSetViewports(1,&viewport);
        const auto shownInputs=assigned|active;
        // Squeeze normals face inward (+X left, -X right). Turn that actual
        // contact surface toward the player rather than behind the housing.
        const float targetYaw=(shownInputs&(1ull<<(7+hand)))?(hand?.95f:-.95f):hand?-.20f:.20f;
        const float targetPitch=(shownInputs&(1ull<<(9+hand)))?.22f:0.f;
        p.yaw[hand]+=(targetYaw-p.yaw[hand])*.15f;
        p.pitch[hand]+=(targetPitch-p.pitch[hand])*.15f;
        image=p.models[hand].texture.Get();context->PSSetShaderResources(0,1,&image);
        for(const auto& part:p.models[hand].parts){
            const bool pressed=(part.inputs&pressedMask)!=0,touched=(part.inputs&touchMask)!=0,chosen=(part.inputs&assigned)!=0;
            
            std::array<float,8> params{std::cos(p.yaw[hand]),std::sin(p.yaw[hand]),std::cos(p.pitch[hand]),std::sin(p.pitch[hand]),1,1,1,0};
            if(pressed){params[4]=.12f;params[5]=.85f;params[6]=.55f;params[7]=.72f;}
            else if(touched){params[4]=.15f;params[5]=.65f;params[6]=1;params[7]=.55f;}
            else if(chosen){params[4]=1;params[5]=.64f;params[6]=.20f;params[7]=.65f;}
            context->UpdateSubresource(constants,0,nullptr,params.data(),0,0);
            const UINT stride=sizeof(ModelVertex),offset=0;auto* vertices=part.vertices.Get();context->IASetVertexBuffers(0,1,&vertices,&stride,&offset);
            context->IASetIndexBuffer(part.indices.Get(),DXGI_FORMAT_R32_UINT,0);context->DrawIndexed(part.count,0,0);
        }
    }
    ID3D11ShaderResourceView* empty{};context->PSSetShaderResources(0,1,&empty);context->OMSetRenderTargets(0,nullptr,nullptr);
    if(p.samples>1)context->ResolveSubresource(p.output.Get(),0,p.color.Get(),0,DXGI_FORMAT_B8G8R8A8_UNORM_SRGB);
    else context->CopyResource(p.output.Get(),p.color.Get());
    ComPtr<ID3D11CommandList> commands;if(FAILED(context->FinishCommandList(FALSE,&commands)))return nullptr;
    immediate->ExecuteCommandList(commands.Get(),TRUE);return p.output.Get();
}
}
