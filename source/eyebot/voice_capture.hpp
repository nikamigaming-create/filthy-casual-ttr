#pragma once
#include <windows.h>
#include <mmsystem.h>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <memory>
#include <vector>
#include "voice_commands.hpp"

namespace robco_voice {
struct Handle {
    HANDLE value;
    explicit Handle(HANDLE value = nullptr) : value(value) {}
    ~Handle() { if (valid()) CloseHandle(value); }
    bool valid() const { return value && value != INVALID_HANDLE_VALUE; }
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;
};

inline std::wstring installedFile(const wchar_t* relativePath) {
    Handle file(CreateFileW(relativePath, FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                            nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
    if (!file.valid()) return {};
    wchar_t path[1024]{};
    const auto length = GetFinalPathNameByHandleW(file.value, path, DWORD(std::size(path)), 0);
    if (!length || length >= std::size(path)) return {};
    std::wstring result(path, length);
    if (result.compare(0, 8, L"\\\\?\\UNC\\") == 0) return L"\\\\" + result.substr(8);
    return result.compare(0, 4, L"\\\\?\\") == 0 ? result.substr(4) : result;
}

class Capture {
    struct Temp {
        wchar_t file[MAX_PATH]{};
        Temp() {
            wchar_t directory[MAX_PATH]{};
            const DWORD length = GetTempPathW(MAX_PATH, directory);
            if (length && length < MAX_PATH) GetTempFileNameW(directory, L"FCV", 0, file);
        }
        ~Temp() { if (*file) DeleteFileW(file); }
    };
    enum class Stage { Starting, Listening, Processing };
    struct Job {
        Handle stop{CreateEventW(nullptr, TRUE, FALSE, nullptr)};
        Handle cancel{CreateEventW(nullptr, TRUE, FALSE, nullptr)};
        std::atomic<Stage> stage{Stage::Starting};
        std::string result, error;
#ifdef FC_VOICE_TEST
        std::filesystem::path fixture;
#endif
        bool cancelled() const { return WaitForSingleObject(cancel.value, 0) == WAIT_OBJECT_0; }
    };
    std::unique_ptr<Job> job;
    HANDLE thread = nullptr;

    static bool record(Job& task, const std::filesystem::path& path) {
        if (task.cancelled()) return false;
        constexpr DWORD sampleRate = 16000;
        constexpr WORD block = 2;
        constexpr WORD sampleBits = 16;
        WAVEFORMATEX format{WAVE_FORMAT_PCM, 1, sampleRate, sampleRate * block, block, sampleBits, 0};
        std::vector<char> samples(sampleRate * block * 6);
        Handle done(CreateEventW(nullptr, FALSE, FALSE, nullptr));
        HWAVEIN input = nullptr;
        if (!done.valid() || waveInOpen(&input, WAVE_MAPPER, &format, DWORD_PTR(done.value), 0, CALLBACK_EVENT)) {
            task.error = "Microphone unavailable";
            return false;
        }
        WAVEHDR buffer{};
        buffer.lpData = samples.data();
        buffer.dwBufferLength = DWORD(samples.size());
        const bool prepared = waveInPrepareHeader(input, &buffer, sizeof(buffer)) == MMSYSERR_NOERROR;
        const bool started = prepared && waveInAddBuffer(input, &buffer, sizeof(buffer)) == MMSYSERR_NOERROR &&
                             waveInStart(input) == MMSYSERR_NOERROR;
        if (started) {
            task.stage = Stage::Listening;
            const HANDLE signals[]{task.cancel.value, task.stop.value, done.value};
            const auto deadline = GetTickCount64() + 6000;
            while (!(buffer.dwFlags & WHDR_DONE) && GetTickCount64() < deadline) {
                const auto signal = WaitForMultipleObjects(DWORD(std::size(signals)), signals, FALSE, 150);
                if (signal == WAIT_OBJECT_0 || signal == WAIT_OBJECT_0 + 1 || signal == WAIT_FAILED) break;
            }
        }
        waveInReset(input);
        if (prepared) waveInUnprepareHeader(input, &buffer, sizeof(buffer));
        waveInClose(input);
        if (task.cancelled()) return false;
        if (!started || buffer.dwBytesRecorded < sampleRate * block / 4) {
            task.error = started ? "No speech recorded" : "Microphone unavailable";
            return false;
        }
        const DWORD bytes = buffer.dwBytesRecorded;
        const DWORD header[]{36 + bytes, 0x45564157, 0x20746D66, 16, 0x00010001,
            sampleRate, sampleRate * block, (DWORD(sampleBits) << 16) | block, 0x61746164, bytes};
        std::ofstream wave(path, std::ios::binary | std::ios::trunc);
        wave.write("RIFF", 4);
        wave.write(reinterpret_cast<const char*>(header), sizeof(header));
        wave.write(samples.data(), bytes);
        wave.close();
        return bool(wave);
    }

    static void recognize(Job& task, const std::wstring& host, const std::wstring& model, const wchar_t* wave) {
        Temp output;
        SECURITY_ATTRIBUTES inherited{sizeof(inherited), nullptr, TRUE};
        Handle out(CreateFileW(output.file, GENERIC_WRITE, FILE_SHARE_READ, &inherited, TRUNCATE_EXISTING,
                               FILE_ATTRIBUTE_TEMPORARY, nullptr));
        Handle in(CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, &inherited,
                              OPEN_EXISTING, 0, nullptr));
        if (!out.valid() || !in.valid()) { task.error = "Voice helper output unavailable"; return; }
        auto command = L"\"" + host + L"\" --test \"" + model + L"\" en 2 \"" + wave + L"\" [_TRANSCRIBE_]";
        STARTUPINFOW startup{};
        startup.cb = sizeof(startup);
        startup.dwFlags = STARTF_USESTDHANDLES;
        startup.hStdInput = in.value;
        startup.hStdOutput = startup.hStdError = out.value;
        PROCESS_INFORMATION process{};
        if (!CreateProcessW(host.c_str(), command.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW,
                            nullptr, nullptr, &startup, &process)) { task.error = "Voice helper could not start"; return; }
        const Handle worker(process.hProcess), initialThread(process.hThread);
        const HANDLE signals[]{task.cancel.value, worker.value};
        const auto signal = WaitForMultipleObjects(DWORD(std::size(signals)), signals, FALSE, 8000);
        if (signal != WAIT_OBJECT_0 + 1) {
            TerminateProcess(worker.value, 1);
            WaitForSingleObject(worker.value, 1000);
            task.error = "Voice recognition timed out";
            return;
        }
        DWORD result = 0;
        if (!GetExitCodeProcess(worker.value, &result) || result != 0) { task.error = "Voice recognition failed"; return; }
        std::ifstream response(std::filesystem::path(output.file), std::ios::binary);
        char data[4096]{};
        response.read(data, sizeof(data));
        task.result = transcript(std::string_view(data, size_t(response.gcount())));
        if (task.result.empty()) task.error = "No speech recognized";
    }

    static DWORD WINAPI run(void* state) noexcept {
        auto& task = *static_cast<Job*>(state);
        try {
            const auto host = installedFile(L"Data\\NVSE\\Plugins\\TTWVR\\ttwvr_voice_host.exe");
            const auto model = installedFile(L"Data\\NVSE\\Plugins\\TTWVR\\voice\\ggml-base-q8_0.bin");
            if (host.empty() || model.empty()) { task.error = "TTR voice files unavailable"; return 0; }
#ifdef FC_VOICE_TEST
            if (!task.fixture.empty()) {
                task.stage = Stage::Processing;
                recognize(task, host, model, task.fixture.c_str());
                return 0;
            }
#endif
            Temp wave;
            if (!*wave.file || !record(task, wave.file)) {
                if (task.error.empty()) task.error = "Could not record audio";
                return 0;
            }
            if (!task.cancelled()) {
                task.stage = Stage::Processing;
                recognize(task, host, model, wave.file);
            }
        } catch (...) { task.error = "Voice input unavailable"; }
        return 0;
    }

public:
    static bool available(){
        return !installedFile(L"Data\\NVSE\\Plugins\\TTWVR\\ttwvr_voice_host.exe").empty()&&
               !installedFile(L"Data\\NVSE\\Plugins\\TTWVR\\voice\\ggml-base-q8_0.bin").empty();
    }
    bool active() const { return thread != nullptr; }
    const char* status() const {
        if (!job) return "Start listening";
        const auto stage = job->stage.load();
        return stage == Stage::Listening ? "Listening - select to finish" :
               stage == Stage::Processing ? "Transcribing" : "Opening microphone";
    }
    bool start(
#ifdef FC_VOICE_TEST
        std::filesystem::path fixture = {}
#endif
    ) {
        if (active()) return false;
        job = std::make_unique<Job>();
#ifdef FC_VOICE_TEST
        job->fixture = std::move(fixture);
#endif
        if (job->stop.valid() && job->cancel.valid()) thread = CreateThread(nullptr, 0, run, job.get(), 0, nullptr);
        if (!thread) job.reset();
        return active();
    }
    void stop() { if (job) SetEvent(job->stop.value); }
    void cancel() { if (job) SetEvent(job->cancel.value); }
    bool poll(std::string& text, std::string& error) {
        if (!thread || WaitForSingleObject(thread, 0) != WAIT_OBJECT_0) return false;
        CloseHandle(thread);
        thread = nullptr;
        if (!job->cancelled()) { text = std::move(job->result); error = std::move(job->error); }
        job.reset();
        return true;
    }
    void shutdown() {
        cancel();
        if (!thread) return;
        WaitForSingleObject(thread, 1500);
        std::string text, error;
        if (!poll(text, error)) {
            CloseHandle(thread);
            thread = nullptr;
            job.release(); // The OS reclaims a stalled worker at process exit.
        }
    }
};
}
