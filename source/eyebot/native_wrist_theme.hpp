#pragma once

#include <windows.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

namespace eyebot::native_theme {

class Theme {
    struct Glyph {
        uint16_t x, y, width, height;
        int16_t bearing, top, advance;
    };
    static_assert(sizeof(Glyph) == 14);
    static constexpr int IconSize = 32;

    struct Surface {
        HDC dc = nullptr;
        HBITMAP bitmap = nullptr;
        HGDIOBJ previous = nullptr;
        COLORREF colour = CLR_INVALID;
        void* pixels = nullptr;

        ~Surface() {
            if (previous) SelectObject(dc, previous);
            if (bitmap) DeleteObject(bitmap);
            if (dc) DeleteDC(dc);
        }
    };

public:
    explicit Theme(const char* path) {
        std::ifstream file(path, std::ios::binary);
        struct Header {uint32_t magic, width, height, line, colour;} header{};
        if (!file.read(reinterpret_cast<char*>(&header), sizeof header) ||
            header.magic != 0x31574346u || header.width < 32 || header.width > 1024 ||
            header.height < 16 || header.height > 1024 || header.line < 8 || header.line > 128 ||
            !file.read(reinterpret_cast<char*>(glyphs.data()), sizeof glyphs)) return;
        for (const auto& glyph : glyphs) {
            if (glyph.advance < 0 || glyph.advance > 256 || std::abs(glyph.top) > 256 ||
                std::abs(glyph.bearing) > 256 || unsigned(glyph.x + glyph.width) > header.width ||
                unsigned(glyph.y + glyph.height) > header.height) return;
        }
        width = int(header.width);
        height = int(header.height);
        line = int(header.line);
        ink = RGB((header.colour >> 24) & 255, (header.colour >> 16) & 255, (header.colour >> 8) & 255);
        mask.resize(size_t(width) * (height + 4 * IconSize));
        if (!file.read(reinterpret_cast<char*>(mask.data()), size_t(width) * height)) return;
        for (int y = 0; y < 4 * IconSize; ++y) {
            if (!file.read(reinterpret_cast<char*>(mask.data() + size_t(height + y) * width), IconSize)) return;
        }
        ready = file.peek() == std::ifstream::traits_type::eof();
    }

    bool available() const { return ready; }
    COLORREF colour() const { return ink; }

    bool text(HDC dc, std::string_view value, RECT box, int size, COLORREF tint, bool wrap = false) {
        auto source = surface(dc, tint);
        if (!source) return false;
        const float scale = float(size) / line;
        const auto limit = box.right - box.left;
        auto y = box.top;
        const auto saved = SaveDC(dc);
        if (!saved) return false;
        IntersectClipRect(dc, box.left, box.top, box.right, box.bottom);
        while (!value.empty() && y + size <= box.bottom) {
            size_t count = 0, lastSpace = std::string_view::npos;
            int advance = 0;
            for (; count < value.size(); ++count) {
                const auto step = glyph(value[count]).advance;
                if (std::lround((advance + step) * scale) > limit) break;
                if (value[count] == ' ') lastSpace = count;
                advance += step;
            }
            const auto crowded = count < value.size();
            if (crowded && wrap && lastSpace != std::string_view::npos && lastSpace != 0) count = lastSpace;
            if (crowded && !wrap) {
                const int dots = 3 * glyph('.').advance;
                while (count && std::lround((advance + dots) * scale) > limit)
                    advance -= glyph(value[--count]).advance;
            }
            if (!count) break;
            const int baseline = y + int(std::lround(size * .78f));
            const int pen = draw(dc, source, value.substr(0, count), box.left, baseline, scale);
            if (!wrap) {
                if (crowded) draw(dc, source, "...", pen, baseline, scale);
                break;
            }
            value.remove_prefix(count);
            while (!value.empty() && value.front() == ' ') value.remove_prefix(1);
            y += size;
        }
        RestoreDC(dc, saved);
        return true;
    }

    bool icon(HDC dc, unsigned index, int x, int y, int size, COLORREF tint) {
        auto source = index < 4 ? surface(dc, tint) : nullptr;
        if (!source) return false;
        return AlphaBlend(dc, x, y, size, size, source, 0, height + int(index) * IconSize,
                          IconSize, IconSize, {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA}) != 0;
    }

private:
    const Glyph& glyph(char value) const { return glyphs[static_cast<unsigned char>(value)]; }

    int draw(HDC dc, HDC source, std::string_view value, int x, int baseline, float scale) const {
        int pen = 0;
        for (char c : value) {
            const auto& item = glyph(c);
            if (item.width && item.height) {
                const int left = x + int(std::lround((pen + item.bearing) * scale));
                const int top = baseline - int(std::lround(item.top * scale));
                AlphaBlend(dc, left, top, std::max(1, int(std::lround(item.width * scale))),
                           std::max(1, int(std::lround(item.height * scale))), source,
                           item.x, item.y, item.width, item.height, {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA});
            }
            pen += item.advance;
        }
        return x + int(std::lround(pen * scale));
    }

    HDC surface(HDC target, COLORREF tint) {
        if (!ready) return nullptr;
        for (const auto& copy : copies) if (copy.colour == tint) return copy.dc;
        auto& copy = copies[next++ % copies.size()];
        if (!copy.dc) copy.dc = CreateCompatibleDC(target);
        if (!copy.dc) return nullptr;
        if (!copy.bitmap) {
            BITMAPINFO info{};
            info.bmiHeader = {sizeof(BITMAPINFOHEADER), width, -(height + 4 * IconSize), 1, 32, BI_RGB};
            copy.bitmap = CreateDIBSection(copy.dc, &info, DIB_RGB_COLORS, &copy.pixels, nullptr, 0);
            if (!copy.bitmap) return nullptr;
            copy.previous = SelectObject(copy.dc, copy.bitmap);
        }
        auto pixels = static_cast<uint32_t*>(copy.pixels);
        const unsigned red = GetRValue(tint), green = GetGValue(tint), blue = GetBValue(tint);
        for (size_t i = 0; i < mask.size(); ++i) {
            const unsigned alpha = mask[i];
            pixels[i] = (alpha << 24) | ((red * alpha / 255) << 16) |
                        ((green * alpha / 255) << 8) | (blue * alpha / 255);
        }
        copy.colour = tint;
        return copy.dc;
    }

    int width = 0, height = 0, line = 0;
    bool ready = false;
    COLORREF ink = RGB(255, 182, 66);
    size_t next = 0;
    std::array<Glyph, 256> glyphs{};
    std::array<Surface, 2> copies{};
    std::vector<unsigned char> mask;
};

inline Theme& installed() {
    static Theme theme("Data/Textures/FilthyCasual/WristTheme.bin");
    return theme;
}

} // namespace eyebot::native_theme
