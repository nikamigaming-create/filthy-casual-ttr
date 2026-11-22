#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d9.h>
#include <cassert>
#include <cstdio>
#include "projected_depth_adapter.hpp"
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "user32.lib")

int main() {
    auto window = CreateWindowExA(0, "STATIC", "Stereo depth test", WS_POPUP,
                                  0, 0, 256, 160, nullptr, nullptr, GetModuleHandle(nullptr), nullptr);
    assert(window);
    auto api = Direct3DCreate9(D3D_SDK_VERSION);
    assert(api);
    D3DPRESENT_PARAMETERS params{};
    params.Windowed = TRUE;
    params.SwapEffect = D3DSWAPEFFECT_DISCARD;
    params.hDeviceWindow = window;
    IDirect3DDevice9* device = nullptr;
    assert(SUCCEEDED(api->CreateDevice(0, D3DDEVTYPE_HAL, window,
                                      D3DCREATE_SOFTWARE_VERTEXPROCESSING, &params, &device)));
    assert(projected_depth_adapter::install(device));
    projected_depth_adapter::requested = true;

    IDirect3DSurface9* color[3]{};
    IDirect3DSurface9* nativeDepth[3]{};
    const unsigned width[] = {160, 160, 208};
    const unsigned height[] = {128, 128, 144};
    for (unsigned eye = 0; eye < 3; ++eye) {
        assert(SUCCEEDED(device->CreateRenderTarget(width[eye], height[eye], D3DFMT_A16B16G16R16F,
                                        D3DMULTISAMPLE_NONE, 0, FALSE, &color[eye], nullptr)));
        assert(SUCCEEDED(device->CreateDepthStencilSurface(width[eye], height[eye], D3DFMT_D24S8,
                                        D3DMULTISAMPLE_NONE, 0, TRUE, &nativeDepth[eye], nullptr)));
    }

    IDirect3DTexture9* lastDepth[3]{};
    auto completeEye = [&](unsigned eye) {
        assert(SUCCEEDED(device->SetDepthStencilSurface(nullptr)));
        assert(SUCCEEDED(device->SetRenderTarget(0, color[eye])));
        assert(SUCCEEDED(device->SetDepthStencilSurface(nativeDepth[eye])));
        projected_depth_adapter::selectWorldDepth(device);
        IDirect3DSurface9* bound = nullptr;
        assert(SUCCEEDED(device->GetDepthStencilSurface(&bound)) && bound);
        IDirect3DTexture9* sampleable = nullptr;
        HRESULT hr = bound->GetContainer(__uuidof(IDirect3DTexture9), reinterpret_cast<void**>(&sampleable));
        bool attached = SUCCEEDED(hr) && sampleable && bound != nativeDepth[eye];
        if (attached) {
            assert(projected_depth_adapter::sampleTexture(bound) == sampleable);
            lastDepth[eye] = sampleable;
        }
        if (sampleable) sampleable->Release();
        bound->Release();
        return attached;
    };
    unsigned sampled = 0;
    for (unsigned frame = 0; frame < 5; ++frame) {
        sampled += completeEye(0);
        sampled += completeEye(1);
    }
    printf("Separate eye depth surfaces: %u of 8 sampled after the initial frame\n", sampled);
    const bool distinct = lastDepth[0] && lastDepth[1] && lastDepth[0] != lastDepth[1];
    unsigned resized = 0;
    for (unsigned frame = 0; frame < 5; ++frame) {
        resized += completeEye(0);
        resized += completeEye(2);
    }
    printf("Eye depth changed size: %u of 9 sampled after the new surface's first pass\n", resized);
    projected_depth_adapter::requested = false;
    const bool restored = !completeEye(0) && !completeEye(2);
    printf("Lamp off uses native depth: %s\n", restored ? "yes" : "no");
    device->SetDepthStencilSurface(nullptr);
    projected_depth_adapter::release();
    for (auto surface : nativeDepth) surface->Release();
    for (auto surface : color) surface->Release();
    device->Release();
    api->Release();
    DestroyWindow(window);
    return sampled == 8 && distinct && resized == 9 && restored ? 0 : 1;
}
