#pragma once
#include "../nvse_function.hpp"

namespace eyebot {

// The native inventory entry and Activate event only observe our authored
// forms. The input layer and the base game's bindings are untouched.
class UserControls {
public:
    enum class Action {
        Close, Stay, Snap, Record, Recall, Save, Delete, Stow, ConfirmDelete, Position,
        Preview, NewTake, Trash, ConfirmTrash
    };

    bool compile(Scripts& source) {
        remote = source.expression(R"(GetFormFromMod "RobCo Eyebot.esp" "000801")");
        device = source.expression(R"(GetFormFromMod "RobCo Eyebot.esp" "000802")");
        onInventory = fc::nvse::loadFunction(source, "Eyebot/OnInventory");
        setup = fc::nvse::loadFunction(source, "Eyebot/BindRemote");
        moveBeside = fc::nvse::loadFunction(source, "Eyebot/Deploy");
        stow = fc::nvse::loadFunction(source, "Eyebot/Stow");
        closeInventory = source.expression("CloseAllMenus");
        return remote && device && onInventory && setup && moveBeside && stow && closeInventory;
    }

    bool registerForGame(Scripts& source, Form* item) {
        return item && source.call(setup, nullptr, nullptr, nullptr, 2, item, onInventory);
    }

    void show(Console& console, unsigned mode) {
        if (mode >= menus.size()) return;
        menu = mode;
        const auto text = std::string(R"(MessageBoxExAlt ({int eyebotChoice} => DispatchEvent "RobCo:EyebotChoice" (ar_Map "choice"::eyebotChoice)) ")")
                          + menus[mode] + '"';
        console.run(text.c_str(), nullptr);
    }

    Action choose(int index) const {
        constexpr Action actions[][5] = {
            {Action::Stay, Action::Snap, Action::Stow, Action::Close, Action::Close},
            {Action::Snap, Action::Record, Action::Position, Action::Stow, Action::Close},
            {Action::Recall, Action::Close, Action::Close, Action::Close, Action::Close},
            {Action::Preview, Action::Save, Action::Delete, Action::Close, Action::Close},
            {Action::ConfirmDelete, Action::Close, Action::Close, Action::Close, Action::Close},
            {Action::Preview, Action::NewTake, Action::Trash, Action::Close, Action::Close},
            {Action::ConfirmTrash, Action::Close, Action::Close, Action::Close, Action::Close},
        };
        return index >= 0 && index < 5 && menu < menus.size() ? actions[menu][index] : Action::Close;
    }

    Script *remote{}, *device{}, *moveBeside{}, *stow{}, *closeInventory{};

private:
    Script *onInventory{}, *setup{};
    static constexpr std::array<const char*, 7> menus{
        "^RobCo Eyebot^Place the recorder|Stay|Take a Snap|Stow|Close",
        "^RobCo Eyebot^Camera parked|Take a Snap|Record|Reposition|Stow|Close",
        "^RobCo Eyebot^Recording|Recall|Close",
        "^RobCo Eyebot^Review take|Preview still|Save MP4|Discard take|Close",
        "^RobCo Eyebot^Discard this unsaved take?|Discard|Keep take",
        "^RobCo Eyebot^MP4 saved|Preview still|New take|Move MP4 to trash|Close",
        "^RobCo Eyebot^Move this MP4 to the local clip trash?|Move to trash|Keep MP4",
    };
    unsigned menu = 0;
};

}
