#pragma once
#include "interaction_provider.hpp"
#include <windows.h>
#include <tlhelp32.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <mutex>

namespace interaction_registry {
class Providers {
    std::array<const fc_interaction::Provider*, 32> entries{};
    std::atomic<size_t> published{};

public:
    // Discovery serializes writers. Provider descriptors live for the process.
    void add(const fc_interaction::Provider* provider) {
        if (!provider || provider->abi != fc_interaction::version ||
            provider->size != sizeof(*provider) || !provider->input) return;
        const auto count = published.load(std::memory_order_relaxed);
        const auto end = entries.begin() + count;
        if (count == entries.size() || std::find(entries.begin(), end, provider) != end) return;
        entries[count] = provider;
        published.store(count + 1, std::memory_order_release);
    }

    unsigned input(const char* name, int hand, int kind, float x, float y) const {
        const auto count = published.load(std::memory_order_acquire);
        unsigned owner = 0;
        for (size_t i = 0; i < count; ++i) {
            // Later providers still receive neutral input to drain a previous hold.
            const auto next = entries[i]->input(name, hand, kind, owner ? 0.f : x, owner ? 0.f : y);
            if (!owner) owner = next;
        }
        return owner;
    }

    void aim(int hand, const fc_interaction::Ray& ray) const {
        const auto count = published.load(std::memory_order_acquire);
        for (size_t i = 0; i < count; ++i) {
            if (const auto callback = entries[i]->aim) callback(hand, &ray);
        }
    }
};

inline Providers providers;

inline void refresh(uint64_t now) {
    static std::mutex discovery;
    static uint64_t lastScan = 0;
    std::lock_guard<std::mutex> lock(discovery);
    if (lastScan && now - lastScan < 1000) return;
    lastScan = now;
    const auto modules = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
    if (modules == INVALID_HANDLE_VALUE) return;
    MODULEENTRY32W module{};
    module.dwSize = sizeof(module);
    for (BOOL valid = Module32FirstW(modules, &module); valid; valid = Module32NextW(modules, &module)) {
        auto query = reinterpret_cast<fc_interaction::Query>(GetProcAddress(module.hModule, "FC_InteractionProviderV1"));
        if (query) providers.add(query());
    }
    CloseHandle(modules);
}
}
