#pragma once

#include <d3d9.h>
#include <wrl/client.h>
#include "wrist_paint.hpp"

namespace eyebot::native_wrist {
// TTR 1.1.7's installed Pip-Boy NIFs share a curved, 56-vertex screen
// surface. The native UV grid occupies the upper-left three quarters.
class Screen {
public:
    bool update(void* root, const wrist_frame::View& view, const unsigned char* photo) {
        if (!build_adapter::heldModule() || !root || !view.generation) return off();
        const auto now = GetTickCount64();
        const bool replaced=source&&(scene::field<void*>(source,0x24)!=data||slot()!=replacement.Get());
        if (root != owner || replaced || now >= nextCheck) {
            nextCheck = now + 250;
            auto node = scene::find(root, "pipboyscreen:0");
            auto geom = node ? scene::field<void*>(node, 0xB8) : nullptr;
            auto prop = node ? scene::field<void*>(node, 0xA8) : nullptr;
            if (!geom || scene::field<unsigned short>(geom, 8) != 56 ||
                !prop || scene::field<unsigned char>(prop, 0x1C) != 0x15) return off();
            auto texture = scene::field<void*>(prop, 0x60);
            auto next = texture ? scene::field<void*>(texture, 0x24) : nullptr;
            if (!next) return off();
            if (root != owner || source != texture || data != next ||
                slot() != replacement.Get()) {
                reset();
                auto native = scene::field<IDirect3DBaseTexture9*>(next, 0x64);
                Microsoft::WRL::ComPtr<IDirect3DDevice9> device;
                if (!native || native->GetType() != D3DRTYPE_TEXTURE ||
                    FAILED(native->GetDevice(device.GetAddressOf())) ||
                    FAILED(device->CreateTexture(Size, Size, 1, D3DUSAGE_DYNAMIC,
                        D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, replacement.GetAddressOf(), nullptr))) return off();
                owner = root;
                source = texture;
                scene::retain(source);
                data = next;
                original = native;
                if (!paint(view, photo)) return off();
                replacement->AddRef();  // Ownership of the renderer's texture slot.
                slot() = replacement.Get();
                native->Release();
            }
        }
        if (!data || scene::field<void*>(source, 0x24) != data ||
            slot() != replacement.Get()) return off();
        return view.generation == painted || paint(view, photo) || off();
    }

    void reset() {
        if (data && source && scene::field<void*>(source, 0x24) == data &&
            slot() == replacement.Get()) {
            original->AddRef();
            slot() = original.Get();
            replacement->Release();
        }
        if (source) scene::release(source);
        owner = source = data = nullptr;
        original.Reset();
        replacement.Reset();
        painted = 0;
    }

private:
    static constexpr int Size = 512, FaceWidth = 385, FaceHeight = 390;
    // Keep the title below the top lip and the button hints above the
    // raised center trim. The full curved face still gets the backing color.
    static constexpr int LeftInset = 9, RightInset = 9, TopInset = 24, BottomInset = 57;
    static constexpr int ContentWidth = FaceWidth - LeftInset - RightInset;
    static constexpr int ContentHeight = FaceHeight - TopInset - BottomInset;
    void* owner = nullptr;
    void* source = nullptr;
    void* data = nullptr;
    Microsoft::WRL::ComPtr<IDirect3DBaseTexture9> original;
    Microsoft::WRL::ComPtr<IDirect3DTexture9> replacement;
    uint64_t nextCheck = 0;
    uint32_t painted = 0;

    IDirect3DBaseTexture9*& slot() { return scene::field<IDirect3DBaseTexture9*>(data, 0x64); }
    bool off() { reset(); return false; }

    bool paint(const wrist_frame::View& view, const unsigned char* photo) {
        auto dc = CreateCompatibleDC(nullptr);
        if (!dc) return false;
        BITMAPINFO info{};
        info.bmiHeader = {sizeof(BITMAPINFOHEADER), wrist_paint::Width, -wrist_paint::Height,
                          1, 32, BI_RGB};
        void* pixels = nullptr;
        auto surface = CreateDIBSection(dc, &info, DIB_RGB_COLORS, &pixels, nullptr, 0);
        if (!surface) { DeleteDC(dc); return false; }
        auto previous = SelectObject(dc, surface);
        wrist_paint::page(dc, view, photo);
        GdiFlush();
        D3DLOCKED_RECT locked{};
        const bool ready = SUCCEEDED(replacement->LockRect(0, &locked, nullptr, D3DLOCK_DISCARD));
        if (ready) {
            for (int y = 0; y < Size; ++y) {
                const int panelY = y - TopInset;
                auto dst = reinterpret_cast<uint32_t*>(static_cast<unsigned char*>(locked.pBits) +
                    y * locked.Pitch);
                std::fill_n(dst, Size, 0xFF000000u);
                if (panelY < 0 || panelY >= ContentHeight) continue;
                const auto src = static_cast<const uint32_t*>(pixels) +
                    (panelY * wrist_paint::Height / ContentHeight) * wrist_paint::Width;
                for (int x = 0; x < ContentWidth; ++x) {
                    dst[LeftInset + x] = src[x * wrist_paint::Width / ContentWidth] | 0xFF000000u;
                }
            }
            replacement->UnlockRect(0);
            painted = view.generation;
        }
        SelectObject(dc, previous);
        DeleteObject(surface);
        DeleteDC(dc);
        return ready;
    }
};
}
