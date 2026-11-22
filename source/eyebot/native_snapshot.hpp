#pragma once

#include <d3d9.h>
#include <wrl/client.h>

namespace eyebot::native_snapshot {

// Observes only the dedicated JIP LN camera screen, using the 1.4.0.525
// NiGeometry texture layout and BSShaderNoLightingProperty texture slot.
inline IDirect3DBaseTexture9* screenTexture(void* root) {
    auto screen = scene::find(root, "RobCoEyebotScreen");
    if (!screen) return nullptr;
    auto property = scene::field<void*>(screen, 0xA8);
    if (!property || scene::field<unsigned char>(property, 0x1C) != 0x15) return nullptr;
    auto texture = scene::field<void*>(property, 0x60);
    auto data = texture ? scene::field<void*>(texture, 0x24) : nullptr;
    return data ? scene::field<IDirect3DBaseTexture9*>(data, 0x64) : nullptr;
}

inline float fromHalf(uint16_t value) {
    const int exponent = (value >> 10) & 31;
    if ((value & 0x8000) || exponent == 31) return 0.f;
    const float fraction = float(value & 1023) / 1024.f;
    return exponent ? std::ldexp(1.f + fraction, exponent - 15) : std::ldexp(fraction, -14);
}

constexpr uint32_t PixelWidth = 256;
constexpr uint32_t PixelHeight = 256;
using Pixels = std::array<unsigned char, PixelWidth * PixelHeight * 4>;
constexpr uint32_t MovieSize = 512;
using MoviePixels = std::array<unsigned char, MovieSize * MovieSize * 4>;

// Only the snap and deferred export paths read the GPU target back.
template<size_t Size> bool read(void* root, std::array<unsigned char, Size>& pixels) {
    using Microsoft::WRL::ComPtr;
    auto source = screenTexture(root);
    if (!source || source->GetType() != D3DRTYPE_TEXTURE) return false;
    auto texture = static_cast<IDirect3DTexture9*>(source);
    ComPtr<IDirect3DDevice9> device;
    ComPtr<IDirect3DSurface9> surface, readback;
    D3DSURFACE_DESC desc{};
    if (FAILED(texture->GetLevelDesc(0, &desc)) || !desc.Width || !desc.Height ||
        uint64_t(desc.Width) * desc.Height * 4 != Size ||
        !(desc.Usage & D3DUSAGE_RENDERTARGET) || FAILED(texture->GetDevice(device.GetAddressOf())) ||
        FAILED(texture->GetSurfaceLevel(0, surface.GetAddressOf())) ||
        FAILED(device->CreateOffscreenPlainSurface(desc.Width, desc.Height, desc.Format,
                 D3DPOOL_SYSTEMMEM, readback.GetAddressOf(), nullptr)) ||
        FAILED(device->GetRenderTargetData(surface.Get(), readback.Get()))) return false;
    const bool half = desc.Format == D3DFMT_A16B16G16R16F;
    if (!half && desc.Format != D3DFMT_X8R8G8B8 && desc.Format != D3DFMT_A8R8G8B8) return false;
    D3DLOCKED_RECT lock{};
    if (FAILED(readback->LockRect(&lock, nullptr, D3DLOCK_READONLY))) return false;
    int darkest = 765, brightest = 0;
    for (unsigned y = 0; y < desc.Height; ++y) {
        const auto bytes = static_cast<const unsigned char*>(lock.pBits) + y * lock.Pitch;
        auto converted = pixels.data() + y * desc.Width * 4;
        for (unsigned x = 0; x < desc.Width; ++x) {
            for (int c = 0; c < 3; ++c) {
                if (!half) converted[x * 4 + c] = bytes[x * 4 + c];
                else {
                    const auto sample = reinterpret_cast<const uint16_t*>(bytes)[x * 4 + 2 - c];
                    converted[x * 4 + c] = static_cast<unsigned char>(255 * std::min(1.f, fromHalf(sample)));
                }
            }
            converted[x * 4 + 3] = 255;
            const auto* pixel = converted + x * 4;
            const int sum = int(pixel[0]) + pixel[1] + pixel[2];
            darkest = std::min(darkest, sum);
            brightest = std::max(brightest, sum);
        }
    }
    readback->UnlockRect();
    return brightest - darkest > 24;
}

inline void thumbnail(const MoviePixels& movie, Pixels& still) {
    static_assert(MovieSize == PixelWidth * 2 && MovieSize == PixelHeight * 2);
    for (uint32_t y = 0; y < PixelHeight; ++y) for (uint32_t x = 0; x < PixelWidth; ++x) {
        const auto src = ((y * 2) * MovieSize + x * 2) * 4;
        const auto dst = (y * PixelWidth + x) * 4;
        for (uint32_t c = 0; c != 4; ++c) {
            still[dst + c] = uint8_t((unsigned(movie[src+c]) + movie[src+4+c] +
                movie[src+MovieSize*4+c] + movie[src+MovieSize*4+4+c] + 2) / 4);
        }
    }
}

inline bool saveProof(void* root, const std::string& path) {
    Pixels pixels;
    if (!read(root, pixels)) return false;
    BITMAPINFOHEADER image{};
    image.biSize = sizeof(image);
    image.biWidth = PixelWidth;
    image.biHeight = -static_cast<LONG>(PixelHeight);
    image.biPlanes = 1;
    image.biBitCount = 32;
    image.biSizeImage = pixels.size();
    BITMAPFILEHEADER file{};
    file.bfType = 0x4d42;
    file.bfOffBits = sizeof(file) + sizeof(image);
    file.bfSize = file.bfOffBits + image.biSizeImage;
    std::ofstream output(path, std::ios::binary);
    output.write(reinterpret_cast<const char*>(&file), sizeof(file));
    output.write(reinterpret_cast<const char*>(&image), sizeof(image));
    output.write(reinterpret_cast<const char*>(pixels.data()), pixels.size());
    return bool(output);
}

}
