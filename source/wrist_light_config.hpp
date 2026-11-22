#pragma once

namespace wrist_light {
struct Config {
    bool enabled = true, projected = true, gloveMount = true;
    float brightness = 1.3f, angle = 32, range = 2450;
    float forward = -3.f, above = 4.2f;
    float rgb[3]{1.f, .93f, .78f};
};
} // namespace wrist_light
