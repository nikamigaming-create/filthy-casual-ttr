#pragma once

#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <condition_variable>
#include <deque>
#include <filesystem>
#include <memory>
#include <mutex>
#include <thread>

namespace eyebot {

class MovieWriter {
public:
    using Pixels = native_snapshot::MoviePixels;
    static constexpr uint32_t FramesPerSecond = 24;
    static constexpr uint32_t Width = native_snapshot::MovieSize;
    static constexpr uint32_t Height = native_snapshot::MovieSize;
    static constexpr uint32_t QueuedFrames = 3;

    ~MovieWriter() { stop(true); }

    bool begin(std::filesystem::path path) {
        if (worker.joinable()) return false;
        ready = false;
        done = false;
        closing = false;
        cancelled = false;
        result = E_PENDING;
        try {
            worker = std::thread(&MovieWriter::run, this, std::move(path));
        } catch (...) {
            done = true;
            result = E_FAIL;
            return false;
        }
        return true;
    }

    bool writable() {
        std::lock_guard<std::mutex> lock(mutex);
        return ready && !closing && !done && queued.size() < QueuedFrames;
    }

    bool write(uint32_t index, std::unique_ptr<Pixels> pixels) {
        std::lock_guard<std::mutex> lock(mutex);
        if (!ready || closing || done || queued.size() == QueuedFrames) return false;
        queued.push_back({index, std::move(pixels)});
        wake.notify_one();
        return true;
    }

    void seal() {
        std::lock_guard<std::mutex> lock(mutex);
        closing = true;
        wake.notify_one();
    }

    bool finished(HRESULT& status) {
        {
            std::lock_guard<std::mutex> lock(mutex);
            if (!done) return false;
            status = result;
        }
        stop(false);
        return true;
    }

    void stop(bool cancel) {
        {
            std::lock_guard<std::mutex> lock(mutex);
            closing = true;
            if (cancel) cancelled = true;
            wake.notify_one();
        }
        if (worker.joinable()) worker.join();
        queued.clear();
    }

private:
    struct Frame { uint32_t index; std::unique_ptr<Pixels> pixels; };
    using YUV = std::array<unsigned char, Width * Height * 3 / 2>;

    static HRESULT mediaType(IMFMediaType* type, REFGUID format) {
        HRESULT status = type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
        if (SUCCEEDED(status)) status = type->SetGUID(MF_MT_SUBTYPE, format);
        if (SUCCEEDED(status)) status = type->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
        if (SUCCEEDED(status)) status = MFSetAttributeSize(type, MF_MT_FRAME_SIZE, Width, Height);
        if (SUCCEEDED(status)) status = MFSetAttributeRatio(type, MF_MT_FRAME_RATE, FramesPerSecond, 1);
        if (SUCCEEDED(status)) status = MFSetAttributeRatio(type, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
        return status;
    }

    static HRESULT create(const wchar_t* path, IMFSinkWriter** result, DWORD& stream) {
        using Microsoft::WRL::ComPtr;
        ComPtr<IMFSinkWriter> writer;
        ComPtr<IMFAttributes> settings;
        ComPtr<IMFMediaType> output, input;
        HRESULT status = MFCreateAttributes(settings.GetAddressOf(), 1);
        if (SUCCEEDED(status)) status = settings->SetUINT32(MF_SINK_WRITER_DISABLE_THROTTLING, TRUE);
        if (SUCCEEDED(status)) status = MFCreateSinkWriterFromURL(path, nullptr, settings.Get(), writer.GetAddressOf());
        if (SUCCEEDED(status)) status = MFCreateMediaType(output.GetAddressOf());
        if (SUCCEEDED(status)) status = mediaType(output.Get(), MFVideoFormat_H264);
        if (SUCCEEDED(status)) status = output->SetUINT32(MF_MT_AVG_BITRATE, 3200000);
        if (SUCCEEDED(status)) status = writer->AddStream(output.Get(), &stream);
        if (SUCCEEDED(status)) status = MFCreateMediaType(input.GetAddressOf());
        if (SUCCEEDED(status)) status = mediaType(input.Get(), MFVideoFormat_NV12);
        if (SUCCEEDED(status)) status = writer->SetInputMediaType(stream, input.Get(), nullptr);
        if (SUCCEEDED(status)) status = writer->BeginWriting();
        if (SUCCEEDED(status)) *result = writer.Detach();
        return status;
    }

    static void convert(const Pixels& colour, unsigned char* target) {
        // Limited-range BT.601 for an SD frame. Average chroma over each 2x2 quad.
        auto uv = target + Width * Height;
        for (uint32_t y = 0; y < Height; y += 2) for (uint32_t x = 0; x < Width; x += 2) {
            int totalB = 0, totalG = 0, totalR = 0;
            for (unsigned dy = 0; dy < 2; ++dy) for (unsigned dx = 0; dx < 2; ++dx) {
                const size_t index = (y + dy) * Width + x + dx;
                const auto src = colour.data() + index * 4;
                const int b = src[0], g = src[1], r = src[2];
                target[index] = uint8_t(((66*r + 129*g + 25*b + 128) >> 8) + 16);
                totalB += b;
                totalG += g;
                totalR += r;
            }
            const size_t at = (y / 2) * Width + x;
            uv[at] = uint8_t(((-38*totalR - 74*totalG + 112*totalB + 512) >> 10) + 128);
            uv[at + 1] = uint8_t(((112*totalR - 94*totalG - 18*totalB + 512) >> 10) + 128);
        }
    }

    static HRESULT encode(IMFSinkWriter* writer, DWORD stream, const Frame& frame) {
        using Microsoft::WRL::ComPtr;
        ComPtr<IMFMediaBuffer> buffer;
        ComPtr<IMFSample> sample;
        HRESULT status = MFCreateMemoryBuffer(static_cast<DWORD>(std::tuple_size<YUV>::value), buffer.GetAddressOf());
        BYTE* data = nullptr;
        if (SUCCEEDED(status)) status = buffer->Lock(&data, nullptr, nullptr);
        if (SUCCEEDED(status)) {
            convert(*frame.pixels, data);
            status = buffer->Unlock();
        }
        if (SUCCEEDED(status)) status = buffer->SetCurrentLength(static_cast<DWORD>(std::tuple_size<YUV>::value));
        if (SUCCEEDED(status)) status = MFCreateSample(sample.GetAddressOf());
        if (SUCCEEDED(status)) status = sample->AddBuffer(buffer.Get());
        const LONGLONG begin = LONGLONG(frame.index) * 10000000 / FramesPerSecond;
        const LONGLONG end = LONGLONG(frame.index + 1) * 10000000 / FramesPerSecond;
        if (SUCCEEDED(status)) status = sample->SetSampleTime(begin);
        if (SUCCEEDED(status)) status = sample->SetSampleDuration(end - begin);
        if (SUCCEEDED(status)) status = writer->WriteSample(stream, sample.Get());
        return status;
    }

    void run(std::filesystem::path file) {
        auto temporary = file;
        temporary.replace_extension(L".work.mp4");
        HRESULT status = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        const bool com = SUCCEEDED(status);
        if (com) status = MFStartup(MF_VERSION);
        const bool media = SUCCEEDED(status);
        Microsoft::WRL::ComPtr<IMFSinkWriter> writer;
        DWORD stream = 0;
        if (media) status = create(temporary.c_str(), writer.GetAddressOf(), stream);
        {
            std::lock_guard<std::mutex> lock(mutex);
            ready = SUCCEEDED(status);
        }
        while (SUCCEEDED(status)) {
            Frame frame{};
            {
                std::unique_lock<std::mutex> lock(mutex);
                wake.wait(lock, [&] { return closing || !queued.empty(); });
                if (cancelled || queued.empty()) break;
                frame = std::move(queued.front());
                queued.pop_front();
            }
            status = encode(writer.Get(), stream, frame);
        }
        if (SUCCEEDED(status)) {
            std::lock_guard<std::mutex> lock(mutex);
            if (cancelled) status = E_ABORT;
        }
        if (SUCCEEDED(status)) status = writer->Finalize();
        writer.Reset();
        if (media) MFShutdown();
        if (com) CoUninitialize();
        {
            std::lock_guard<std::mutex> lock(mutex);
            if (cancelled) status = E_ABORT;
            if (SUCCEEDED(status) && !MoveFileExW(temporary.c_str(), file.c_str(), MOVEFILE_WRITE_THROUGH)) {
                status = HRESULT_FROM_WIN32(GetLastError());
            }
            if (FAILED(status)) DeleteFileW(temporary.c_str());
            result = status;
            done = true;
        }
    }

    std::mutex mutex;
    std::condition_variable wake;
    std::deque<Frame> queued;
    std::thread worker;
    bool ready = false, done = false, closing = false, cancelled = false;
    HRESULT result = E_PENDING;
};

}
