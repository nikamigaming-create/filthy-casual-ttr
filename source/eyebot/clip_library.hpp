#pragma once

namespace eyebot::clips {

inline bool movieName(const std::wstring& name) {
    if (name.size() != 30 || name.compare(0, 7, L"Eyebot-") ||
        name[15] != L'-' || name[22] != L'-' || name.compare(26, 4, L".mp4")) return false;
    for (unsigned i = 7; i != 26; ++i)
        if (i != 15 && i != 22 && (name[i] < L'0' || name[i] > L'9')) return false;
    return true;
}

inline bool ordinary(const std::filesystem::path& path, bool directory) {
    const auto attrs = GetFileAttributesW(path.c_str());
    return attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_REPARSE_POINT) &&
           bool(attrs & FILE_ATTRIBUTE_DIRECTORY) == directory;
}

inline std::vector<std::filesystem::path> list(const std::filesystem::path& directory) {
    std::vector<std::filesystem::path> found;
    if (!ordinary(directory, true)) return found;
    std::error_code error;
    for (std::filesystem::directory_iterator it(directory, error), end; !error && it != end; it.increment(error)) {
        const auto& candidate = it->path();
        if (movieName(candidate.filename().wstring()) && ordinary(candidate, false) &&
            it->file_size(error) >= 1024 && !error) found.push_back(candidate);
    }
    std::sort(found.begin(), found.end(), std::greater<>());
    return found;
}

inline bool trash(const std::filesystem::path& path) {
    if (!ordinary(path, false) || !ordinary(path.parent_path(), true)) return false;
    const auto directory = path.parent_path() / ".eyebot-trash";
    std::error_code error;
    std::filesystem::create_directory(directory, error);
    if (error || !ordinary(directory, true)) return false;
    const auto target = directory / path.filename();
    if (!MoveFileExW(path.c_str(), target.c_str(), MOVEFILE_WRITE_THROUGH)) return false;
    auto still = path;
    still.replace_extension(L".dds");
    if (ordinary(still, false)) MoveFileExW(still.c_str(), (directory / still.filename()).c_str(), MOVEFILE_WRITE_THROUGH);
    return true;
}

inline void green(const native_snapshot::Pixels& pixels, native_snapshot::Pixels& toned,
                  bool pilot = false) {
    for (size_t i = 0; i < pixels.size(); i += 4) {
        const unsigned grey = (19 * pixels[i] + 183 * pixels[i+1] + 54 * pixels[i+2]) >> 8;
        if (pilot) {
            // Keep daylight midtone separation through the XR colour conversion.
            const auto level = 10 + (grey * 141u + 127u) / 255u;
            toned[i] = uint8_t(level / 5);
            toned[i+1] = uint8_t(level);
            toned[i+2] = uint8_t(level / 7);
            toned[i+3] = 255;
            continue;
        }
        const auto bright = std::min(255u, (grey * 3) / 2 + 5);
        toned[i] = uint8_t(bright / 3);
        toned[i+1] = uint8_t(bright);
        toned[i+2] = uint8_t(bright / 4);
        toned[i+3] = 255;
    }
}

inline bool still(const std::filesystem::path& path, const native_snapshot::Pixels& pixels) {
    constexpr auto width = native_snapshot::PixelWidth;
    constexpr auto height = native_snapshot::PixelHeight;
    const std::array<uint32_t, 32> dds{
        0x20534444,124,0x100F,height,width,width*4,0,0,
        0,0,0,0,0,0,0,0,0,0,0,
        32,0x40,0,32,0x00ff0000,0x0000ff00,0x000000ff,0,
        0x1000,0,0,0,0
    };
    auto toned = std::make_unique<native_snapshot::Pixels>();
    green(pixels, *toned);
    std::ofstream output(path, std::ios::binary);
    output.write(reinterpret_cast<const char*>(dds.data()), sizeof(dds));
    output.write(reinterpret_cast<const char*>(toned->data()), toned->size());
    return bool(output);
}

} // namespace eyebot::clips
