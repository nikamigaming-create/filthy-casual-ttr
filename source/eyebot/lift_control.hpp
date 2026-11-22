#pragma once
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>

namespace eyebot::flight {
class LiftControl {
    struct Trigger {
        std::atomic<float> pressure{};
        std::atomic<uint64_t> sample{};
        bool consumed{}; // Accessed only by the input thread.
    } triggers[2];

  public:
    bool update(int hand, float raw, bool piloting, uint64_t now) {
        auto &trigger = triggers[hand];
        constexpr float deadzone = .18f;
        const auto pressure = std::isfinite(raw) ? std::clamp(raw, 0.f, 1.f) : 0.f;
        const bool held = pressure > deadzone;
        if (piloting) {
            trigger.pressure = held ? (pressure - deadzone) / (1.f - deadzone) : 0.f;
            trigger.sample = now;
            trigger.consumed = held;
            return true;
        }
        trigger.sample = 0;
        const bool drain = trigger.consumed;
        trigger.consumed = drain && held;
        return drain;
    }

    float axis(uint64_t now) const {
        auto freshPressure = [now](const Trigger &trigger) {
            const auto at = trigger.sample.load();
            return at && now >= at && now - at < 180 ? trigger.pressure.load() : 0.f;
        };
        return freshPressure(triggers[1]) - freshPressure(triggers[0]);
    }
};
} // namespace eyebot::flight
