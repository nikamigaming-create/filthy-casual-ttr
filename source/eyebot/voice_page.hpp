#pragma once
#include "../nvse_function.hpp"
#include "voice_capture.hpp"
#include "../build_adapter.hpp"
#include "../nvse_api.hpp"
#include "../settings_ui.hpp"
#include <cmath>
#include <cstring>

namespace robco_voice {
class Page {
    Scripts* scripts = nullptr;
    Arrays* arrays = nullptr;
    Script *inventory{}, *name{}, *count{}, *equip{}, *health{}, *ap{}, *equipped{}, *weaponOut{};
    Script *light{}, *lightOn{}, *lightOff{};
    Capture capture;
    Form* pending = nullptr;
    std::string heard, answer;
    int lightTarget = -1;
    uint64_t lightCheckAt = 0;

    Element query(Script* script, Form* item = nullptr) const {
        Element result;
        if (item) scripts->call(script, nullptr, nullptr, &result, 1, item);
        else scripts->call(script, nullptr, nullptr, &result, 0);
        return result;
    }

    std::string itemLabel(Form* item) const {
        const auto label = query(name, item);
        if (label.type != 3 || !label.value.string) return {};
        std::string result(label.value.string);
        build_adapter::freeGameString(label.value.string);
        return result;
    }

    std::string numberLabel(Element value) const {
        return value.type == 1 && std::isfinite(value.value.number) ? std::to_string(std::llround(value.value.number)) : "Unavailable";
    }

    void findItem(const std::string& term, bool equipping) {
        const auto list = query(inventory);
        const auto size = list.type == 4 && list.value.array ? arrays->size(list.value.array) : 0;
        if (!size || size > 4096) { answer = size ? "Inventory unavailable" : "No matching item"; return; }
        std::vector<Element> items(size);
        if (!arrays->elements(list.value.array, items.data(), nullptr)) { answer = "Inventory unavailable"; return; }
        Form* best = nullptr;
        std::string bestLabel, otherLabel;
        int bestRank = 0;
        for (const auto& entry : items) {
            if (entry.type != 2 || !entry.value.form) continue;
            auto* item = entry.value.form;
            if (equipping && item->type != 24 && item->type != 40) continue;
            auto label = itemLabel(item);
            const auto rank = matchRank(term, label);
            if (!rank || rank < bestRank) continue;
            if (rank > bestRank) {
                best = item;
                bestRank = rank;
                bestLabel = std::move(label);
                otherLabel.clear();
            } else if (item != best) otherLabel = std::move(label);
        }
        if (!best) { answer = equipping ? "No matching weapon or outfit" : "No matching item"; return; }
        if (!otherLabel.empty()) {
            answer = bestLabel == otherLabel ? "Multiple items share that name. Use inventory." : "Which one: " + bestLabel + " or " + otherLabel + "?";
            return;
        }
        if (equipping) { pending = best; answer = "Equip " + bestLabel + "?"; }
        else answer = bestLabel + ": " + numberLabel(query(count, best));
    }

public:
    bool compile(Scripts& source, Arrays& data) {
        scripts = &source; arrays = &data;
        inventory = source.expression("player.GetAllItems 0 1");
        name = fc::nvse::loadFunction(source, "Voice/ItemName");
        count = fc::nvse::loadFunction(source, "Voice/ItemCount");
        equip = fc::nvse::loadFunction(source, "Voice/Equip");
        health = source.expression("player.GetAV Health");
        ap = source.expression("player.GetAV ActionPoints");
        equipped = source.expression("player.GetEquippedObject 5");
        weaponOut = source.expression("player.IsWeaponOut");
        light = source.expression("TogglePipBoyLight");
        lightOn = fc::nvse::loadFunction(source, "Voice/LightOn");
        lightOff = fc::nvse::loadFunction(source, "Voice/LightOff");
        return inventory && name && count && equip && health && ap && equipped && weaponOut;
    }

    void respond(std::string text) {
        pending = nullptr;
        const auto intent = interpret(text);
        for (auto& ch : text) if (ch < ' ' || ch > '~' || ch == '\\' || ch == '"') ch = ' ';
        heard = std::move(text);
        switch (intent.action) {
            case Action::Help: answer = "Health, AP, item counts, equip or flashlight."; break;
            case Action::Equip: findItem(intent.item, true); break;
            case Action::Count: findItem(intent.item, false); break;
            case Action::Health: answer = "Health: " + numberLabel(query(health)); break;
            case Action::ActionPoints: answer = "Action points: " + numberLabel(query(ap)); break;
            case Action::Equipped: {
                const auto item = query(equipped), out = query(weaponOut);
                answer = item.type == 2 && item.value.form ? itemLabel(item.value.form) : "No weapon equipped";
                if (item.type == 2 && item.value.form && out.type == 1 && !out.value.number) answer += " (holstered)";
                break;
            }
            case Action::LightOn:
            case Action::LightOff: {
                const bool on = intent.action == Action::LightOn;
                if (!light || !lightOn || !lightOff) { answer = "Flashlight control unavailable"; break; }
                query(on ? lightOn : lightOff);
                lightTarget = on;
                lightCheckAt = GetTickCount64() + 700;
                answer = "Changing flashlight";
                break;
            }
            case Action::Unknown: answer = "Command not recognized. Try 'what can you do?'"; break;
        }
    }

    void activate(
#ifdef FC_VOICE_TEST
        std::filesystem::path fixture = {}
#endif
    ) {
        if (lightTarget >= 0) return;
        if (pending) {
            const auto label = itemLabel(pending);
            const auto result = query(equip, pending);
            answer = result.type == 1 && result.value.number > 0 ? "Equipped " + label : "Could not equip " + label;
            pending = nullptr;
            return;
        }
        if (capture.active()) { capture.stop(); return; }
        heard.clear(); answer.clear();
        if (!capture.start(
#ifdef FC_VOICE_TEST
            std::move(fixture)
#endif
        )) answer = "Voice input could not start";
    }

    void poll(bool visible) {
        if (visible && lightTarget >= 0 && GetTickCount64() >= lightCheckAt) {
            const auto current = query(light);
            answer = current.type == 1 && int(current.value.number != 0) == lightTarget ?
                     (lightTarget ? "Flashlight on" : "Flashlight off") : "Flashlight did not change";
            lightTarget = -1;
        }
        std::string text, error;
        if (!capture.poll(text, error) || !visible) return;
        if (!text.empty()) respond(std::move(text));
        else if (!error.empty()) answer = std::move(error);
    }

    fc_settings::View view() const {
        fc_settings::View v;
        v.page = fc_settings::WristVoice;
        strcpy_s(v.title, "ROBCO VOICE TERMINAL");
        v.count = 2;
        strcpy_s(v.rows[0].label, pending ? "Confirm equipment" : lightTarget >= 0 ? "Wait for flashlight" : capture.status());
        strcpy_s(v.rows[0].value, pending ? "EQUIP" : lightTarget >= 0 ? "WAIT" : capture.active() ? "STOP" : "TALK");
        v.rows[1].kind = fc_settings::Unavailable;
        strncpy_s(v.rows[1].label, heard.empty() ? "How many stimpaks do I have?" : heard.c_str(), _TRUNCATE);
        strcpy_s(v.rows[1].value, heard.empty() ? "EXAMPLE" : "HEARD");
        strncpy_s(v.note, answer.empty() ? "Select to use your Windows microphone." : answer.c_str(), _TRUNCATE);
        return v;
    }

    void cancel() { capture.cancel(); pending = nullptr; lightTarget = -1; heard.clear(); answer.clear(); }
    void shutdown() { capture.shutdown(); }
};
}
