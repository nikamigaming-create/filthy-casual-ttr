#pragma once
#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>

namespace robco_voice {
enum class Action { Unknown, Help, Equip, Count, Health, ActionPoints, Equipped, LightOn, LightOff };
struct Intent { Action action; std::string item; };

inline std::string normalize(std::string_view text) {
    std::string words;
    for (const unsigned char ch : text) {
        if (std::isalnum(ch)) words += char(std::tolower(ch));
        else if (!words.empty() && words.back() != ' ') words += ' ';
    }
    if (!words.empty() && words.back() == ' ') words.pop_back();
    return words;
}

inline std::string itemName(std::string text) {
    for (const std::string_view article : {"my ", "the "})
        if (text.compare(0, article.size(), article) == 0) return text.substr(article.size());
    return text;
}

inline Intent interpret(std::string_view text) {
    auto words = normalize(text);
    for (const std::string_view address : {"hey pip boy ", "pip boy ", "pipboy ", "robco "}) {
        if (words.compare(0, address.size(), address) != 0) continue;
        words.erase(0, address.size());
        break;
    }
    if (words == "help" || words == "what can you do") return {Action::Help, {}};
    if (words == "health" || words == "how much health do i have" || words == "what is my health" || words == "what s my health")
        return {Action::Health, {}};
    if (words == "action points" || words == "how many action points do i have" || words == "what s my ap" || words == "what is my ap")
        return {Action::ActionPoints, {}};
    if (words == "what am i holding" || words == "what do i have equipped" || words == "what is equipped")
        return {Action::Equipped, {}};
    if (words == "flashlight on" || words == "turn on the flashlight" || words == "turn the flashlight on")
        return {Action::LightOn, {}};
    if (words == "flashlight off" || words == "turn off the flashlight" || words == "turn the flashlight off")
        return {Action::LightOff, {}};
    constexpr std::string_view suffix = " do i have";
    if (words.compare(0, 9, "how many ") == 0 && words.size() > 9 + suffix.size() &&
        words.compare(words.size()-suffix.size(), suffix.size(), suffix) == 0)
        return {Action::Count, itemName(words.substr(9, words.size()-9-suffix.size()))};
    for (const auto prefix : {"equip ", "switch to ", "put on "}) {
        const std::string_view verb(prefix);
        if (words.compare(0, verb.size(), verb) == 0 && words.size() > verb.size())
            return {Action::Equip, itemName(words.substr(verb.size()))};
    }
    return {Action::Unknown, {}};
}

inline std::string itemKey(std::string_view name) {
    auto key = normalize(name);
    for (const auto& [spoken, shortName] : {std::pair{"nine millimeter", "9mm"},
                                          std::pair{"ten millimeter", "10mm"},
                                          std::pair{"millimeter", "mm"}}) {
        if (const auto at = key.find(spoken); at != key.npos) key.replace(at, std::string_view(spoken).size(), shortName);
    }
    key.erase(std::remove(key.begin(), key.end(), ' '), key.end());
    return key;
}

inline int matchRank(std::string_view query, std::string_view candidate) {
    const auto a = itemKey(query), b = itemKey(candidate);
    if (a.empty() || b.empty()) return 0;
    if (a == b) return 3;
    if (a == b + "s" || a == b + "es" || b == a + "s") return 2;
    const auto item = " " + normalize(candidate) + " ";
    return item.find(" " + normalize(query) + " ") != std::string::npos ? 1 : 0;
}

inline std::string transcript(std::string_view output) {
    constexpr std::string_view mark = "| heard \"";
    auto begin = output.find(mark);
    if (begin == output.npos) return {};
    begin += mark.size();
    const auto end = output.find("\" -> choice ", begin);
    if (end == output.npos || end - begin > 180) return {};
    return std::string(output.substr(begin, end - begin));
}
}
