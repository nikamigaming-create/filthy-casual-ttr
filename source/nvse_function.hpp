#pragma once
#include <cstddef>
#include <string>
#include "nvse_api.hpp"

namespace fc::nvse {
// Resolve through the game's virtual Data tree so the winning MO2 override runs.
inline Script* loadFunction(Scripts& api, const char* relativeName) {
    const std::string command = std::string("CompileScript \"FilthyCasual/") + relativeName + ".txt\"";
    auto loader = api.expression(command.c_str());
    Element result{};
    if (!loader || !api.call(loader, nullptr, nullptr, &result, 0) || result.type != 2) return nullptr;
    return reinterpret_cast<Script*>(result.value.form);
}
}
