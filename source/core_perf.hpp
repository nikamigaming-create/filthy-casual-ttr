#pragma once
#include <windows.h>
#include <algorithm>
#include <fstream>
#include <string>
#include <vector>

// Private test-mailbox diagnostics. Never samples ordinary play.
namespace core_perf {
inline bool enabled = false;
inline std::vector<double> samples;
inline unsigned emptyPolls = 0, speakerQueries = 0, mouthMeshes = 0;
inline LONGLONG frequency = 0;

inline void begin() {
    LARGE_INTEGER value{};
    QueryPerformanceFrequency(&value);
    frequency = value.QuadPart;
    samples.clear();
    samples.reserve(20000);
    emptyPolls = speakerQueries = mouthMeshes = 0;
    enabled = true;
}

struct Scope {
    LONGLONG start = 0;
    Scope() {
        if (!enabled) return;
        LARGE_INTEGER value{};
        QueryPerformanceCounter(&value);
        start = value.QuadPart;
    }
    ~Scope() {
        if (!start || !enabled || !frequency || samples.size() >= 20000) return;
        LARGE_INTEGER value{};
        QueryPerformanceCounter(&value);
        samples.push_back(double(value.QuadPart - start) * 1000 / frequency);
    }
};

inline void finish(const std::string& path) {
    enabled = false;
    // Sorting happens once at test completion, outside the measured frame loop.
    auto sorted = samples;
    std::sort(sorted.begin(), sorted.end());
    double sum = 0;
    for (double sample : sorted) sum += sample;
    std::ofstream out(path);
    out << "{\"samples\":" << sorted.size()
        << ",\"meanMs\":" << (sorted.empty() ? 0 : sum / sorted.size())
        << ",\"p95Ms\":" << (sorted.empty() ? 0 : sorted[(sorted.size() - 1) * 95 / 100])
        << ",\"maxMs\":" << (sorted.empty() ? 0 : sorted.back())
        << ",\"emptyPolls\":" << emptyPolls
        << ",\"speakerQueries\":" << speakerQueries
        << ",\"mouthMeshes\":" << mouthMeshes << "}";
}
}
