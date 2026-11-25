#pragma once
#include "comfort_controls.hpp"
#include "feature_setup.hpp"
#include "settings_ui.hpp"
#include "sprint_controls.hpp"
#include "wrist_light_config.hpp"
#include <cstdio>
#include <cstring>
#include <string>

namespace fc_settings::pages {
constexpr int featuresPerWristPage = 5;
struct WristFeatureRows {
    int ids[featuresPerWristPage]{};
    int count = 0, pages = 1;
};
inline WristFeatureRows wristFeatureRows(uint32_t available, int page) {
    WristFeatureRows rows;
    int visible = 0;
    for (int id = 0; id < fc_features::Count; ++id) {
        if (!(available & fc_features::bit(id)))
            continue;
        if (visible / featuresPerWristPage == page)
            rows.ids[rows.count++] = id;
        ++visible;
    }
    rows.pages = std::max(1, (visible + featuresPerWristPage - 1) / featuresPerWristPage);
    return rows;
}

inline std::string fixed(float value, int digits = 1) {
    char buffer[40];
    sprintf_s(buffer, "%.*f", digits, value);
    return buffer;
}

inline const char *on(bool enabled) { return enabled ? "ON" : "OFF"; }

inline void add(View &view, const char *label, const std::string &value, Kind kind, const char *detail) {
    auto &row = view.rows[view.count++];
    strcpy_s(row.label, label);
    strcpy_s(row.value, value.c_str());
    strcpy_s(row.detail, detail);
    row.kind = kind;
}

inline void wristHome(View &v, const char *seat, bool canReload, bool manual,
                      bool tutorialAvailable, bool tutorialEnabled) {
    strcpy_s(v.title, "GENERAL SETTINGS");

    add(v, "Reload mode", canReload ? (manual ? "MANUAL" : "AUTOMATIC") : "UNAVAILABLE",
        canReload ? fc_settings::Toggle : fc_settings::Unavailable,
        canReload ? "Switch between manual and automatic reload. Your choice is saved."
                  : "The native TTR reload setting is unavailable.");
    add(v, "Seated height", seat, fc_settings::Action, "Adjust seated height for your current seat.");
    add(v, "Sprint", "OPEN", fc_settings::Action, "Speed, AP drain, activation and notifications.");
    add(v, "Wrist flashlight", "OPEN", fc_settings::Action, "Brightness, range, colour and position.");
    add(v, "Hands and movement", "OPEN", fc_settings::Action,
        "Finger tracking, grip sensitivity, smoothing and walking.");
    add(v, "Features", "OPEN", fc_settings::Action, "Choose optional features and save them together.");
    add(v, "Controller mapping", "OPEN", fc_settings::Action, "Edit and save your control profiles.");
    add(v, "Weapon tutorial", tutorialAvailable ? on(tutorialEnabled) : "UNAVAILABLE",
        tutorialAvailable ? fc_settings::Toggle : fc_settings::Unavailable,
        "Show an interaction guide when holding a supported weapon.");
}

inline void features(View &v, const fc_features::Selection &wristFeatures, int wristFeaturePage) {
    const auto featureRows = wristFeatureRows(wristFeatures.available, wristFeaturePage);
    sprintf_s(v.title, "FEATURES %d / %d", wristFeaturePage + 1, featureRows.pages);
    for (int index = 0; index < featureRows.count; ++index) {
        const int id = featureRows.ids[index];
        const auto &definition = fc_features::definitions[id];
        add(v, definition.label, on(wristFeatures.pending & fc_features::bit(id)), fc_settings::Toggle,
            definition.detail);
    }
    add(v, "Previous page", "OPEN", fc_settings::Action, "Go to the previous group of features.");
    add(v, "Next page", "OPEN", fc_settings::Action, "Go to the next group of features.");
    add(v, "Apply changes", "SAVE", fc_settings::Action,
        "Apply this selection. Vehicle and other add-on changes may require a restart.");
}

inline void remapping(View &v) {
    strcpy_s(v.title, "CONTROLLER MAPPING");
    add(v, "Open remapping", "OPEN", fc_settings::Action, "Edit a binding or save a control profile.");
}

inline void home(View &v, const char *seat) {
    strcpy_s(v.title, "FILTHY CASUALS / SETTINGS");
    add(v, "Seated height", seat, fc_settings::Action,
        "Adjust your current seat. On-foot and car seat heights save separately.");
    add(v, "Sprint", "OPEN", fc_settings::Action,
        "Speed, AP use, stick activation and the sprint notification.");
    add(v, "Wrist flashlight", "OPEN", fc_settings::Action,
        "Brightness, reach, beam width, colour and lamp position.");
    add(v, "Feature selection", "OPEN", fc_settings::Action, "Choose which optional features are enabled.");
    add(v, "Controller mapping", "OPEN", fc_settings::Action,
        "Edit button mappings. Enable Controller Remapping in Feature Selection to apply them.");
    add(v, "Hands and movement", "OPEN", fc_settings::Action,
        "Optional Index fingers, grip sensitivity, hand smoothing and walking controls.");
    add(v, "Save and close", "CLOSE", fc_settings::Action, "Close settings and return to the game.");
}

inline void height(View &v, const char *seat, int seatContext, float heightOffset, bool showSelectorPopups) {
    sprintf_s(v.title, "SEATED HEIGHT / %s", seat);
    add(v, "Height adjustment", fixed(heightOffset * 100, 0) + " cm", fc_settings::Number,
        "Left/right or -/+ adjusts eye height by 1 cm. Saved for this seat.");
    add(v, "Calibrate ground height", seatContext ? "ON FOOT ONLY" : "CALIBRATE",
        seatContext ? fc_settings::Unavailable : fc_settings::Action,
        "Match your target eye height while seated on foot. In a vehicle, use Height adjustment.");
    add(v, "Reset adjustment", "RESET", fc_settings::Action,
        "Restore zero height adjustment for this seat only.");
    add(v, "Selector messages", on(showSelectorPopups), fc_settings::Toggle,
        "Show item-selector status messages. This does not hide height-save confirmations.");
}

inline void sprint(View &v, const sprint_controls::Config &sprintConfig, bool sprintNoticeEnabled,
                   bool available) {
    strcpy_s(v.title, "SPRINT");
    add(v, "Sprint enabled", on(sprintConfig.enabled), fc_settings::Toggle,
        "Sprint when the movement stick reaches your activation point. Requires Sprint in Feature "
        "Selection.");
    add(v, "Sprint speed", fixed(sprintConfig.multiplier, 2) + "x", fc_settings::Number,
        "Multiplier of normal movement speed while sprinting.");
    add(v, "AP drain", fixed(sprintConfig.apPerSecond, 0) + " / second", fc_settings::Number,
        "Action Points used each second. Zero allows sprinting without AP cost.");
    add(v, "Start at stick position", fixed(sprintConfig.start * 100, 0) + "%", fc_settings::Number,
        "100% is the forward edge of the stick. Lower values start sprinting earlier.");
    add(v, "Show sprint notification", on(sprintNoticeEnabled), fc_settings::Toggle,
        "Briefly show Sprinting when a sprint starts.");
    if (!available)
        strcpy_s(v.note, "Sprint is disabled in Feature Selection. These preferences are still saved.");
}

inline void light(View &v, const wrist_light::Config &c, bool available) {
    strcpy_s(v.title, "WRIST FLASHLIGHT");
    add(v, "Wrist flashlight", on(c.enabled), fc_settings::Toggle,
        "Use the wrist-guided light when the native flashlight is switched on.");
    add(v, "Brightness", fixed(c.brightness), fc_settings::Number,
        "Increase or decrease the light intensity.");
    add(v, "Beam width", fixed(c.angle, 0) + " degrees", fc_settings::Number,
        "A wider beam covers more of the area in front of your wrist.");
    add(v, "Reach", fixed(c.range / 69.99125f, 0) + " metres", fc_settings::Number,
        "Maximum light distance. The beam fades with distance.");
    const char *colour = c.rgb[0] < .8f   ? "GREEN"
                         : c.rgb[1] < .8f ? "AMBER"
                         : c.rgb[2] < .9f ? "WARM WHITE"
                                          : "WHITE";
    add(v, "Light colour", colour, fc_settings::Number, "Choose warm white, white, green or amber.");
    add(v, "Lamp position", "OPEN", fc_settings::Action,
        "Adjust the beam's position relative to your wrist.");
    if (!available)
        strcpy_s(v.note, "Wrist Light is disabled in Feature Selection. These preferences are still saved.");
}

inline void comfort(View &v) {
    strcpy_s(v.title, "HANDS AND MOVEMENT");
    add(v, "Hands", "OPEN", fc_settings::Action,
        "Individual Index fingers, squeeze thresholds and positional hand/item smoothing.");
    add(v, "Movement zones", "OPEN", fc_settings::Action, "Set where walking, running and sprinting begin.");
}

inline void hands(View &v, const fc_comfort::Config &c, bool fresh) {
    strcpy_s(v.title, "HANDS");
    add(v, "Index finger tracking", c.fingers ? "AUTO" : "OFF", fc_settings::Toggle,
        "Uses a live Index skeleton when the runtime provides one. Native item contacts keep priority.");
    add(v, "Custom grip sensitivity", on(c.grip), fc_settings::Toggle,
        "Native grab sensitivity only. Existing holds stay intact; new thresholds start after release.");
    add(v, "Grip engages at", fixed(c.press * 100, 0) + "%", fc_settings::Number,
        "Lower values need less squeeze. Applies to left and right grip after remapping.");
    add(v, "Grip releases at", fixed(c.release * 100, 0) + "%", fc_settings::Number,
        "A lower release threshold prevents accidental drops. Must stay below engagement.");
    add(v, "Hand smoothing", on(c.smoothing), fc_settings::Toggle,
        "Filters native grip and aim together on foot. Does not change native contact poses. Off in menus "
        "and vehicles.");
    add(v, "Smoothing amount", fixed(c.smoothMs, 0) + " ms", fc_settings::Number,
        "1 to 30 ms. Position only; native wrist rotation stays unchanged.");

    strcpy_s(v.note, !c.fingers ? "Index tracking is off."
                     : fresh    ? "Index skeletal samples received. Native grips retain priority."
                                : "No live Index skeleton. Existing finger gestures remain available.");
}

inline void walking(View &v, const fc_comfort::Config &c) {
    strcpy_s(v.title, "MOVEMENT ZONES");
    add(v, "Movement zones", on(c.walking), fc_settings::Toggle,
        "Adjust on-foot movement. Vehicles and menus keep their own controls.");
    add(v, "Walk starts at", fixed(c.deadzone * 100, 0) + "%", fc_settings::Number,
        "Stick travel below this point is neutral.");
    add(v, "Run starts at", fixed(c.runStart * 100, 0) + "%", fc_settings::Number,
        "Normal running begins here. Default 60%.");
    add(v, "Sprint starts at", fixed(c.sprintStart * 100, 0) + "%", fc_settings::Number,
        "Forward stick travel required to sprint. Enable Sprint in Feature Selection.");
    add(v, "Walking pace", fixed(c.walkSpeed * 100, 0) + "%", fc_settings::Number,
        "Changes the pace inside the walking zone.");
    add(v, "Allow auto-walk", on(c.autoWalk), fc_settings::Toggle,
        "Bind Auto-walk in Controller Mapping. Manual movement, menus or tracking loss stops it.");
    strcpy_s(v.note, "Auto-walk needs Controller Remapping enabled and a binding in the ON FOOT profile.");
}

inline void lampPosition(View &v, const wrist_light::Config &c) {
    strcpy_s(v.title, "WRIST FLASHLIGHT / POSITION");
    add(v, "Forward offset", fixed(c.forward / 69.99125f * 100) + " cm", fc_settings::Number,
        "Move the beam origin toward or away from your knuckles.");
    add(v, "Upward offset", fixed(c.above / 69.99125f * 100) + " cm", fc_settings::Number,
        "Move the beam origin above the wrist.");
}
} // namespace fc_settings::pages
