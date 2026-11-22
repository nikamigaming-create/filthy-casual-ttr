#pragma once
#include <wincodec.h>
#include <wrl/client.h>
#include <filesystem>
#include <vector>
#include <map>
#include <string>
#pragma comment(lib,"windowscodecs.lib")
#pragma comment(lib,"ole32.lib")
// Shared, locally generated RobCo faceplates. Live text is never baked into art.
namespace robco_theme {
constexpr COLORREF phosphor=RGB(173,245,139),secondary=RGB(145,191,158);
inline const char* font="Segoe UI";
inline std::map<std::string,std::vector<unsigned char>> cache;
inline bool background(void* destination,int width,int height,bool compact){
 const auto key=std::to_string(width)+"x"+std::to_string(height)+(compact?"-compact":"-terminal");
 auto it=cache.find(key);if(it==cache.end()){
  using Microsoft::WRL::ComPtr;auto hr=CoInitializeEx(nullptr,COINIT_MULTITHREADED);struct COM{HRESULT result;~COM(){if(SUCCEEDED(result))CoUninitialize();}} com{hr};
  wchar_t modulePath[MAX_PATH]{};auto module=GetModuleHandleW(L"SeatedInput.dll");if(!module)return false;GetModuleFileNameW(module,modulePath,MAX_PATH);
  auto path=std::filesystem::path(modulePath).parent_path()/L"Controls"/(compact?L"robco-compact.png":L"robco-terminal.png");
  ComPtr<IWICImagingFactory> factory;ComPtr<IWICBitmapDecoder> decoder;ComPtr<IWICBitmapFrameDecode> frame;ComPtr<IWICBitmapScaler> scaler;ComPtr<IWICFormatConverter> converter;
  if(FAILED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&factory)))||FAILED(factory->CreateDecoderFromFilename(path.c_str(),nullptr,GENERIC_READ,WICDecodeMetadataCacheOnLoad,&decoder))||FAILED(decoder->GetFrame(0,&frame))||FAILED(factory->CreateBitmapScaler(&scaler))||FAILED(scaler->Initialize(frame.Get(),width,height,WICBitmapInterpolationModeFant))||FAILED(factory->CreateFormatConverter(&converter))||FAILED(converter->Initialize(scaler.Get(),GUID_WICPixelFormat32bppBGRA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom)))return false;
  std::vector<unsigned char> pixels(size_t(width)*height*4);if(FAILED(converter->CopyPixels(nullptr,width*4,UINT(pixels.size()),pixels.data())))return false;it=cache.emplace(key,std::move(pixels)).first;
 }memcpy(destination,it->second.data(),it->second.size());return true;
}
}
