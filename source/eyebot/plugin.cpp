#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <shlobj.h>

#include <array>
#include <atomic>
#include <cstddef>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <string>

#include "../nvse_api.hpp"
#include "../interaction_provider.hpp"
#include "native_scene.hpp"
#include "native_snapshot.hpp"
#include "recorder_camera.hpp"
#include "lift_control.hpp"
#include "pilot_motion.hpp"
#include "scene_replay.hpp"
#include "movie_writer.hpp"
#include "user_controls.hpp"
#include "wrist_page.hpp"
#include "equipment_menu.hpp"
#include "equipment_picture.hpp"
#include "build_catalog.hpp"
#include "clip_library.hpp"
#include "wrist_frame.hpp"
#include "native_wrist.hpp"
#include "native_wrist_frame.hpp"
#include "filthy_wrist_hardware.hpp"
#include "filthy_native_tabs.hpp"
#include "voice_page.hpp"
#include "../settlement-vr/crew.hpp"
#include "../enclave-vr/fort_messages.hpp"
#include "../settlement-vr/ui.hpp"

namespace {

#if defined(FC_SETTINGS_ONLY)
constexpr bool settingsOnly = true;
#else
constexpr bool settingsOnly = false;
#endif

// xNVSE 6 EventManager ABI through SetNativeEventHandler (interface 8).
struct Events {
    bool (__cdecl* registerEvent)(const char*, uint8_t, uint8_t*, uint32_t);
    void* dispatch;
    void* dispatchAlternate;
    bool (__cdecl* listen)(const char*, void (__cdecl*)(Form*, void*));
};

Scripts* scripts = nullptr;
Arrays* arrays = nullptr;
Console* gameConsole = nullptr;
Script* getPlayer = nullptr;
Script* getCell = nullptr;
Script* getReferences = nullptr;
Script* menuMode = nullptr;
Script* findDevices = nullptr;
eyebot::RecorderCamera recorderCamera;
eyebot::Replay replay;
eyebot::Snapshot snapshot;
eyebot::MovieWriter movie;
std::filesystem::path moviePath;
uint32_t movieFrame = 0, movieFrames = 0;
uint32_t restoredFrames = 0;
int movieStage = 0;
bool movieSaved = false, previewRequested = false;
uint32_t lensRef = 0;
uint64_t lensEpoch = 0;
uint64_t nextSample = 0;
uint64_t clockHz = 0;
uint64_t maxSampleTime = 0;
uint64_t samplingTime = 0;
uint32_t sampleCalls = 0;
bool loaded = false;
std::string qaDirectory;
eyebot::UserControls controls;
bool recorderInstalled = !settingsOnly;
bool controlScripts = false, controlsBound = false, showCommands = false, summonRequested = false;
uint64_t showDeadline = 0;
uint32_t authoredDeviceId = 0;
eyebot::WristPage wrist;
std::atomic_bool wristReady = false;
bool wristBound = false, wristDeployPending = false;
bool returnToBuild = false;
std::atomic_bool pilotActive = false;
eyebot::flight::LiftControl liftControl;
std::atomic_bool wristInputAvailable = false, wristInputOpen = false;
std::atomic_bool fullPipboyReady{};
std::atomic<ULONGLONG> filthyPointer{};
std::atomic<uint16_t> fullFilthyPageState{};
std::atomic_uint fullPipboyGrips{}, fullPageGripArmed{}, pointedButton{};
std::atomic_bool categoryTriggerHeld{},compactSettings{};
std::atomic_uint visibleButtons{};
constexpr unsigned PreviousFullPage=2048,NextFullPage=4096;
constexpr unsigned SelectFilthyTab=16, SelectGeneral=32, SelectEyebot=64, SelectVoice=128, SelectPool=256, SelectWorkshop=512, SelectEquipment=1024;
constexpr unsigned SelectFilthyPages=SelectGeneral|SelectEyebot|SelectVoice|SelectPool|SelectWorkshop|SelectEquipment;
std::atomic_bool liveWristOpen = false;
std::atomic_bool nativeWristReady = false;
std::atomic<uint64_t> liveWristAt{}, focusedWristAt{};
bool onArm = false;
eyebot::native_wrist::Screen nativeWrist;
eyebot::FilthyWristHardware wristHardware;
std::mutex wristImageMutex;
eyebot::wrist_frame::View wristImage;
eyebot::native_snapshot::Pixels wristPhotograph{};
bool wristHasPhoto = false;
eyebot::native_snapshot::Pixels pilotPhotograph{}, equipmentPhotograph{};
eyebot::equipment::MenuTexture equipmentTexture;
fc_equipment::LayoutView shownLayout{};
fc_equipment::Controllers shownControllers{};
bool controllerPicture=false;
std::string layoutTextureName;
const auto& devicePicture(){
    if(wristImage.settings)return equipmentPhotograph;
    return wristImage.piloting?pilotPhotograph:wristPhotograph;
}
// TTR supplies this buffer to its normal wrist texture uploader. Both its
// desktop and XR consumers receive the same content at source.
void paintNativeWrist(uint32_t* output,int width,int height) {
    std::lock_guard<std::mutex> lock(wristImageMutex);
    static eyebot::wrist_paint::NativeFrame frame;
    const auto& photo=devicePicture();
    nativeWristReady=frame.copy(wristImage,photo.data(),output,width,height);
}
bool pilotFrameReady = false;
uint64_t nextPilotFrame = 0;
uint32_t pilotFrames = 0;
std::atomic<unsigned> wristInputEdges = 0;
std::atomic<uint64_t> stickTime[2]{};
std::atomic<float> stick[2][2]{};
bool positioned = false;
scene::Vec parkOrigin{};
float flightYaw = 0.f, flightPitch = 0.f;
std::vector<std::filesystem::path> library;
size_t libraryIndex = 0;
std::filesystem::path previewFile;
unsigned previewSequence = 0;
int wristSelection = eyebot::WristPage::Deploy;
std::atomic_bool wristSettingsInput=false;
std::atomic_bool wristEyebotInput=false;
bool deviceSettings=false, deviceEditor=false;
robco_voice::Page voice;
bool voiceOpen = false, voiceReady = false, settlementOpen = false;
unsigned settlementRow = 0, crewOffset = 0;
enum class SettlementView { Home, Roster, Orders, Jobs, Troops, DirectoryIndex, Directory, Faction, CompareList, Comparison, Catalog, Notices };
unsigned fortNoticeToken=0;int fortTextPage=0,fortChoicePage=0;
fort_messages::Access fortMessages(){
 const auto module=GetModuleHandleW(L"FilthyCasualEnclave.dll");
 return module?reinterpret_cast<fort_messages::Access>(GetProcAddress(module,"FC_FortMessages")):nullptr;
}
eyebot::build_catalog::Browser buildCatalog;
SettlementView settlementView = SettlementView::Home;
settlement_crew::Page crewPage;
uint32_t crewResident = 0;
std::string crewName, crewMessage;
bool companyGuard=false;
unsigned originSlot=0;
std::vector<settlement_crew::Range> directoryTrail,originTrail;
int directoryRegion=0,originRegion=0;
std::string otherFactionName;

bool crewRequest(int action, unsigned id, unsigned index, settlement_crew::Page& out) {
    const auto module = GetModuleHandleW(L"FilthyCasualSettlements.dll");
    const auto call = module ? reinterpret_cast<settlement_crew::Access>(GetProcAddress(module,"FC_SettlementCrew")) : nullptr;
    return call && call(action,id,index,&out,sizeof(out));
}
bool crewAction(int action,unsigned index,settlement_crew::Page& out){return crewRequest(action,crewResident,index,out);}

bool fieldMenu(unsigned offset,int choice,settlement_crew::Page& out) {
    const auto module = GetModuleHandleW(L"FilthyCasualEnclave.dll");
    using Read = bool (__cdecl*)(unsigned,int,settlement_crew::Page*,unsigned);
    const auto read = module ? reinterpret_cast<Read>(GetProcAddress(module,"FC_WristFieldMenu")) : nullptr;
    return read && read(offset,choice,&out,sizeof(out));
}
bool fieldMode(int action) {
    const auto module = GetModuleHandleW(L"FilthyCasualEnclave.dll");
    const auto request = module ? reinterpret_cast<bool(__cdecl*)(int)>(GetProcAddress(module,"FC_WristFieldMode")) : nullptr;
    return request && request(action);
}
bool settlementDesk(int action) {
    const auto module = GetModuleHandleW(L"FilthyCasualSettlements.dll");
    const auto request = module ? reinterpret_cast<bool(__cdecl*)(int)>(GetProcAddress(module,"FC_SettlementDesk")) : nullptr;
    return request && request(action);
}
fc_settings::View deviceView;
fc_settings::Navigation deviceNavigation;
std::atomic<float> settingsX{},settingsY{};
std::atomic<uint64_t> settingsAt{};
std::atomic_bool settingsStickDrain=false;

bool poolPageOpen=false,equipmentOpen=false;
eyebot::equipment::Menu equipment;
fc_settings::DeviceSettings poolSettingsProvider(){
    const auto module=GetModuleHandleW(L"FilthyCasualPool.dll");
    return module?reinterpret_cast<fc_settings::DeviceSettings>(GetProcAddress(module,"FC_PoolSettings")):nullptr;
}
bool settingsCommand(unsigned command,int row=0,int direction=0,fc_settings::View* out=nullptr) {
    if(equipmentOpen)return equipment.command(command,row,direction,out,out?sizeof(*out):0);
    if(poolPageOpen){
        const auto apply=poolSettingsProvider();
        return apply&&apply(command,row,direction,out,out?sizeof(*out):0);
    }
    const auto module=GetModuleHandleW(L"SeatedControls.dll");
    const auto apply=module?reinterpret_cast<fc_settings::DeviceSettings>(GetProcAddress(module,"FC_WristSettings")):nullptr;
    return apply&&apply(command,row,direction,out,out?sizeof(*out):0);
}
bool editorCommand(unsigned command,fc_settings::View& view,int row=0) {
    const auto module=GetModuleHandleW(L"SeatedInput.dll");
    const auto apply=module?reinterpret_cast<fc_settings::DeviceEditor>(GetProcAddress(module,"FC_WristEditor")):nullptr;
    return apply&&apply(command,row,&view,sizeof(view));
}

std::unique_ptr<eyebot::native_snapshot::Pixels> movieStill;

void refreshWrist();
void showWristStill(const std::filesystem::path&);
void closeWrist(bool restoreCategory=true);
void publishWristImage(const char* status, const char* help, const char* clip = "");

constexpr size_t ReferenceLimit = 8192;
std::array<Element, ReferenceLimit> references;
std::array<eyebot::Entity, eyebot::Replay::MaxActors> sampleActors;
std::array<eyebot::native_scene::RigFrame, eyebot::Replay::MaxActors> sampleRigs;
eyebot::native_scene::RecordedRigs playerRigs;

Element call(Script* script, Form* caller = nullptr) {
    Element value;
    if (script) scripts->call(script, caller, nullptr, &value, 0);
    return value;
}

Form* form(Element value) {
    return value.type == 2 ? value.value.form : nullptr;
}

Element argument(void* parameters, const char* field) {
    if (!parameters) return {};
    auto arguments = arrays->lookup(uint32_t(reinterpret_cast<uintptr_t>(*static_cast<void**>(parameters))));
    Element key, value;
    key.type = 3;
    key.value.string = const_cast<char*>(field);
    if (!arguments || !arrays->get(arguments, key, value)) return {};
    return value;
}

Form* deviceFrom(void* parameters) {
    auto device = form(argument(parameters, "device"));
    // TESObjectREFR, not an arbitrary TESForm from a script argument.
    return device && device->type == 0x3A ? device : nullptr;
}

uint64_t microseconds() {
    LARGE_INTEGER counter{};
    QueryPerformanceCounter(&counter);
    const auto ticks = static_cast<uint64_t>(counter.QuadPart);
    return (ticks / clockHz) * 1000000 + ((ticks % clockHz) * 1000000) / clockHz;
}

void reset() {
    returnToBuild = false;
    closeWrist();
    wristDeployPending = positioned = false;
    wristInputAvailable = false;
    wristHardware.reset();
    eyebot::native_tabs::request=0;
    eyebot::native_tabs::fullMenu=0;
    eyebot::native_tabs::wornAvailable=false;
    fullPipboyReady = false;
    wristInputEdges = 0;
    wrist.available = false;
    onArm = false;
    wrist.selected = nullptr;
    wrist.nextTabCheck = wrist.lastTick = 0;
    movieStill.reset();
    movie.stop(true);
    replay = {};
    playerRigs.clear();
    snapshot.cancel();
    ++lensEpoch;
    lensRef = 0;
    nextSample = 0;
    nextPilotFrame = 0;
    pilotFrameReady = false;
    pilotFrames = 0;
    showCommands = summonRequested = false;
    movieSaved = previewRequested = false;
}

Form* findDevice() {
    // The authored recorder can be in the exterior persistent cell while
    // GetFormRefs enumerates a different set of currently loaded references.
    const auto authored = form(call(controls.device));
    if (authored && authored->id == lensRef) return authored;
    const auto devices = call(findDevices);
    if (devices.type != 4 || !devices.value.array) return nullptr;
    const auto size = arrays->size(devices.value.array);
    if (!size || size > ReferenceLimit || !arrays->elements(devices.value.array, references.data(), nullptr)) {
        return nullptr;
    }
    for (unsigned i = 0; i < size; ++i) {
        auto reference = form(references[i]);
        if (reference && reference->id == lensRef) return reference;
    }
    return nullptr;
}

void refreshPilotCamera(Form* device, Form* player, uint64_t now) {
    if (now < nextPilotFrame || !device || !player || device->id != lensRef ||
        !scene::root(device) || scene::cell(device) != scene::cell(player)) return;
    scene::Transform pose{};
    static eyebot::native_snapshot::Pixels pixels;
    const auto start = microseconds();
    const bool ready = recorderCamera.project(*scripts,device, replay.lens(), pose) == 1 &&
        eyebot::native_snapshot::read(scene::root(device), pixels);
    const auto elapsed = microseconds() - start;
    // Limit both the refresh rate and the proportion of game time spent offering
    // camera guidance. A slow render or a stream of stick inputs cannot queue more work.
    nextPilotFrame = GetTickCount64() + std::max<uint64_t>(900, elapsed / 50);
    if (!ready) return;
    {
        std::lock_guard<std::mutex> lock(wristImageMutex);
        eyebot::clips::green(pixels, pilotPhotograph, true);
        pilotFrameReady = true;
        wristImage.hasStill = 1;
        ++wristImage.generation;
    }
    if (!qaDirectory.empty()) {
        std::ofstream output(qaDirectory + "\\eyebot-pilot-camera.json");
        output << "{\"frames\":" << ++pilotFrames << ",\"renderUs\":" << elapsed
               << ",\"eye\":[" << pose.p.x << ',' << pose.p.y << ',' << pose.p.z
               << "],\"direction\":[" << pose.r[0][0] << ',' << pose.r[1][0]
               << ',' << pose.r[2][0] << "]}\n";
    }
}

bool placeLens(Form* device) {
    auto root = scene::root(device);
    auto mount = scene::find(root, "RobCoEyebotLens");
    if (!mount || !replay.place(eyebot::native_scene::lens(scene::world(mount)))) return false;
    lensRef = device->id;
    ++lensEpoch;
    return true;
}

void __cdecl place(Form*, void* arguments) {
    if (!loaded) return;
    if (auto device = deviceFrom(arguments)) placeLens(device);
}

void __cdecl stay(Form*, void* arguments) {
    auto device = deviceFrom(arguments);
    if (!loaded || !device || device->id != lensRef || replay.state() != eyebot::State::Placed) return;
    // The lens is sampled when Stay is chosen, after the player has moved it.
    if (placeLens(device)) replay.park();
}

void startRecording(Form* device) {
    if (!loaded || !device || device->id != lensRef || !scene::root(device)) return;
    auto player = form(call(getPlayer));
    auto cell = form(call(getCell));
    if (!player || !cell || scene::cell(player) != scene::cell(device)) return;
    const auto now = microseconds();
    if (replay.record(now, cell->id)) {
        playerRigs.clear();
        nextSample = now;
        movieSaved = false;
        maxSampleTime = samplingTime = sampleCalls = 0;
    }
}

void __cdecl start(Form*, void* arguments) { startRecording(deviceFrom(arguments)); }

void __cdecl recall(Form*, void* arguments) {
    auto device = deviceFrom(arguments);
    if (!loaded || !device || device->id != lensRef) return;
    replay.recall();
    if (!qaDirectory.empty()) {
        std::ofstream output(qaDirectory + "\\eyebot-native-capture.json");
        const auto& frames = replay.frames();
        output << "{\"source\":\"native game\",\"frames\":" << frames.size();
        if (frames.empty()) {
            output << ",\"state\":" << static_cast<int>(replay.state()) << "}\n";
            return;
        }
        output << ",\"durationUs\":" << frames.back().timeUs
               << ",\"samplesAttempted\":" << sampleCalls
               << ",\"maxSampleUs\":" << maxSampleTime
               << ",\"avgSampleUs\":" << (sampleCalls ? samplingTime / sampleCalls : 0)
               << ",\"entities\":[";
        auto& actors = replay.actors();
        const auto& frame = frames.back();
        for (uint32_t index = 0; index < frame.count; ++index) {
            if (index) output << ',';
            const auto& item = actors[frame.first + index];
            float span = 0;
            for (const auto& oldest : actors) {
                if (oldest.id != item.id) continue;
                span = eyebot::length(item.pose.position - oldest.pose.position);
                break;
            }
            output << "{\"id\":" << item.id << ",\"asset\":" << item.asset
                   << ",\"span\":" << span
                   << ",\"joints\":" << item.jointCount
                   << ",\"position\":[" << item.pose.position.x << ',' << item.pose.position.y
                   << ',' << item.pose.position.z << "]}";
        }
        output << "]}\n";
    }
}

void takeSnapshot(Form* device) {
    if (!loaded || !device || device->id != lensRef || !findDevices || !recorderCamera.hasStill()) return;
    const auto state = replay.state();
    if (state == eyebot::State::Placed || state == eyebot::State::Parked) {
        snapshot.request(replay.lens(), lensEpoch);
    }
}

void __cdecl requestSnapshot(Form*, void* arguments) { takeSnapshot(deviceFrom(arguments)); }

void renderSnapshot(bool canRender) {
    eyebot::SnapTicket ticket;
    if (!snapshot.take(ticket, canRender)) return;
    bool rendered = false;
    bool proof = false;
    int status = -3;
    scene::Transform cameraPose{};
    if (ticket.lensEpoch == lensEpoch) {
        auto device = findDevice();
        auto player = form(call(getPlayer));
        if (device && device->id == lensRef) status = -5;
        if (status == -5 && scene::root(device)) status = -6;
        if (status == -6 && player && scene::cell(device) == scene::cell(player)) {
            status = recorderCamera.project(*scripts,device, ticket.view, cameraPose);
            rendered = status == 1;
            if (rendered && wrist.open) {
                auto pixels = std::make_unique<eyebot::native_snapshot::Pixels>();
                if (eyebot::native_snapshot::read(scene::root(device), *pixels)) {
                    if (onArm) {
                        std::lock_guard<std::mutex> lock(wristImageMutex);
                        eyebot::clips::green(*pixels, wristPhotograph);
                        wristHasPhoto = true;
                        if (!wristImage.piloting) wristImage.hasStill = 1;
                        ++wristImage.generation;
                    } else {
                        showWristStill({});
                        if (!previewFile.empty() && eyebot::clips::still(previewFile, *pixels)) {
                            const auto texture = std::string("RobCoEyebot/") + previewFile.filename().string();
                            wrist.preview(*gameConsole, texture);
                        }
                    }
                }
            }
            if (rendered && !qaDirectory.empty()) {
                proof = eyebot::native_snapshot::saveProof(scene::root(device), qaDirectory + "\\eyebot-native-snapshot.bmp");
            }
        }
    }
    snapshot.finish(ticket, lensEpoch, rendered);
    if (!qaDirectory.empty()) {
        std::ofstream output(qaDirectory + "\\eyebot-native-snapshot.json");
        output << "{\"ticket\":" << ticket.id << ",\"status\":" << status
               << ",\"rendered\":" << (rendered ? "true" : "false")
               << ",\"cameraPosition\":[" << cameraPose.p.x << ',' << cameraPose.p.y << ',' << cameraPose.p.z << ']'
               << ",\"cameraForward\":[" << cameraPose.r[0][0] << ',' << cameraPose.r[1][0] << ',' << cameraPose.r[2][0] << ']'
               << ",\"savedProof\":" << (proof ? "true" : "false") << "}\n";
    }
}

std::filesystem::path clipDirectory() {
    if (!qaDirectory.empty()) return std::filesystem::path(qaDirectory) / "eyebot-library";
    wchar_t documents[MAX_PATH]{};
    if (FAILED(SHGetFolderPathW(nullptr, CSIDL_PERSONAL, nullptr, SHGFP_TYPE_CURRENT, documents))) return {};
    return std::filesystem::path(documents) / "My Games" / "FalloutNV" / "Eyebot Clips";
}

std::filesystem::path newMoviePath() {
    if (!qaDirectory.empty() && !wrist.open) return std::filesystem::path(qaDirectory) / "eyebot-native-export.mp4";
    auto directory = clipDirectory();
    std::error_code error;
    std::filesystem::create_directories(directory, error);
    if (error || !eyebot::clips::ordinary(directory, true)) return {};
    SYSTEMTIME time{};
    GetLocalTime(&time);
    wchar_t name[80]{};
    swprintf_s(name, L"Eyebot-%04u%02u%02u-%02u%02u%02u-%03u.mp4", time.wYear, time.wMonth,
        time.wDay, time.wHour, time.wMinute, time.wSecond, time.wMilliseconds);
    return directory / name;
}

void saveRecording(Form* device) {
    if (!loaded || !device || !recorderCamera.hasMovie() || device->id != lensRef || replay.state() != eyebot::State::Review) return;
    const auto& frames = replay.frames();
    if (frames.size() < 2) return;
    moviePath = newMoviePath();
    if (moviePath.empty() || !movie.begin(moviePath)) return;
    movieFrame = restoredFrames = 0;
    movieStill.reset();
    movieStage = 0;
    const auto duration = frames.back().timeUs - frames.front().timeUs;
    movieFrames = uint32_t(duration * eyebot::MovieWriter::FramesPerSecond / 1000000) + 1;
    replay.save();
}

void __cdecl save(Form*, void* arguments) { saveRecording(deviceFrom(arguments)); }

void finishMovie(HRESULT reason) {
    movieSaved = SUCCEEDED(reason);
    replay.finishExport(SUCCEEDED(reason));
    if (movieSaved && movieStill && eyebot::clips::movieName(moviePath.filename().wstring())) {
        auto path = moviePath;
        path.replace_extension(L".dds");
        eyebot::clips::still(path, *movieStill);
    }
    movieStill.reset();
    if (wrist.open) refreshWrist();
    if (!qaDirectory.empty()) {
        std::ofstream trace(qaDirectory + "\\eyebot-export-status.json");
        trace << "{\"frames\":" << movieFrame << ",\"restoredFrames\":" << restoredFrames
              << ",\"stage\":" << movieStage << ",\"status\":" << reason << "}\n";
    }
}

bool renderTake(uint64_t recordedAt, eyebot::MovieWriter::Pixels* pixels = nullptr) {
    auto device = findDevice();
    auto player = form(call(getPlayer));
    auto cell = form(call(getCell));
    const auto& sequence = replay.frames();
    bool rendered = false;
    movieStage = 1;
    if (device && player && cell && cell->id == sequence.front().sceneGeneration && scene::root(device)) {
        movieStage = 2;
        const auto referencesArray = call(getReferences);
        const auto size = referencesArray.type == 4 && referencesArray.value.array
            ? arrays->size(referencesArray.value.array) : ReferenceLimit + 1;
        if (size <= ReferenceLimit && (!size || arrays->elements(referencesArray.value.array, references.data(), nullptr))) {
            movieStage = 3;
            eyebot::native_scene::ReplayFrame frame;
            scene::Transform camera{};
            if (frame.apply(replay, recordedAt, player, references.data(), size, playerRigs)) {
                movieStage = 4;
                const auto eye = replay.lens().eye;
                frame.revealPlayerHead(player, scene::Vec{eye.x, eye.y, eye.z} *
                    eyebot::native_scene::UnitsPerMeter);
                if (recorderCamera.project(*scripts,device, replay.lens(), camera, pixels != nullptr) == 1) {
                    movieStage = 5;
                    rendered = !pixels || eyebot::native_snapshot::read(scene::root(device), *pixels);
                }
            } else if (!qaDirectory.empty()) {
                std::ofstream failure(qaDirectory + "\\eyebot-export-apply-error.txt");
                failure << frame.applyError;
            }
            if (!frame.restore()) {
                if (!qaDirectory.empty()) {
                    std::ofstream failure(qaDirectory + "\\eyebot-export-restore-error.txt");
                    failure << frame.mismatch;
                }
                movieStage = 6;
                rendered = false;
            }
        }
    }
    return rendered;
}

void processMovie() {
    if (replay.state() != eyebot::State::Exporting) return;
    HRESULT status{};
    if (movie.finished(status)) { finishMovie(status); return; }
    if (!movie.writable() || movieFrame == movieFrames) return;
    auto pixels = std::make_unique<eyebot::MovieWriter::Pixels>();
    const auto recordedAt = replay.frames().front().timeUs + uint64_t(movieFrame) * 1000000 /
        eyebot::MovieWriter::FramesPerSecond;
    if (!renderTake(recordedAt, pixels.get())) {
        movie.stop(true);
        finishMovie(E_FAIL);
        return;
    }
    if (movieFrame == movieFrames / 2) {
        movieStill = std::make_unique<eyebot::native_snapshot::Pixels>();
        eyebot::native_snapshot::thumbnail(*pixels, *movieStill);
    }
    if (movie.write(movieFrame, std::move(pixels))) {
        ++restoredFrames;
        if (++movieFrame == movieFrames) movie.seal();
    }
}

void __cdecl discard(Form*, void* arguments) {
    auto device = deviceFrom(arguments);
    if (loaded && device && device->id == lensRef) replay.discard();
}

void wantCommands(bool summon) {
    showCommands = true;
    summonRequested = summon;
    showDeadline = GetTickCount64() + 4500;
}

void traceCommands(const char* phase, Form* device = nullptr) {
    if (qaDirectory.empty()) return;
    std::ofstream trace(qaDirectory + "\\eyebot-native-commands.json");
    trace << "{\"phase\":\"" << phase << "\",\"ref\":" << (device ? device->id : 0)
          << ",\"state\":" << int(replay.state()) << "}\n";
}

void __cdecl nativeActivate(Form*, void* arguments) {
    if (!loaded || !controlsBound || !authoredDeviceId || !arguments) return;
    const auto args = static_cast<Form**>(arguments);
    // The public xNVSE OnActivate event passes the target and the activator.
    if (args[0] && args[1] && args[0]->id == authoredDeviceId && args[1]->id == 0x14) {
        traceCommands("activated", args[0]);
        wantCommands(false);
    }
}

void __cdecl remoteCommands(Form*, void*) {
    traceCommands(loaded && controlsBound ? "remote" : "unbound");
    if (!loaded || !controlsBound) return;
    call(controls.closeInventory);
    wantCommands(true);
}

void populateCommands() {
    if (!showCommands || GetTickCount64() > showDeadline) { showCommands = false; return; }
    auto stock = form(call(controls.device));
    auto player = form(call(getPlayer));
    if (!stock || !player) { traceCommands("missing stock or player", stock); return; }
    if (summonRequested) {
        summonRequested = false;
        if (replay.state() == eyebot::State::Recording) replay.recall();
        scripts->call(controls.moveBeside, nullptr, nullptr, nullptr, 1, stock);
    }
    if (!scene::root(stock) || scene::cell(stock) != scene::cell(player)) {
        traceCommands(scene::root(stock) ? "different cell" : "awaiting stock model", stock);
        return;
    }
    if (replay.state() == eyebot::State::Off && !placeLens(stock)) {
        traceCommands("awaiting stock lens", stock);
        return;
    }
    showCommands = false;
    switch (replay.state()) {
        case eyebot::State::Placed: controls.show(*gameConsole, 0); break;
        case eyebot::State::Parked: controls.show(*gameConsole, 1); break;
        case eyebot::State::Recording: controls.show(*gameConsole, 2); break;
        case eyebot::State::Review: controls.show(*gameConsole, movieSaved ? 5 : 3); break;
        default: break;
    }
    traceCommands("opened", stock);
}

bool trashMovie() {
    if (!movieSaved || !eyebot::clips::trash(moviePath)) return false;
    movieSaved = false;
    return true;
}

void showWristStill(const std::filesystem::path& source) {
    if (onArm) {
        std::lock_guard<std::mutex> lock(wristImageMutex);
        wristHasPhoto = false;
        if (eyebot::clips::ordinary(source, false)) {
            std::ifstream image(source, std::ios::binary);
            image.seekg(128);
            image.read(reinterpret_cast<char*>(wristPhotograph.data()), wristPhotograph.size());
            wristHasPhoto = bool(image);
        }
        return;
    }
    const std::filesystem::path folder = "Data/Textures/RobCoEyebot";
    std::error_code error;
    std::filesystem::create_directories(folder, error);
    if (error || !eyebot::clips::ordinary(folder, true)) return;
    auto previous = previewFile;
    previewFile = folder / ("Wrist-" + std::to_string(GetCurrentProcessId()) + "-" +
        std::to_string(++previewSequence) + ".dds");
    wrist.preview(*gameConsole, "RobCoEyebot/screen.dds");
    if (!previous.empty()) DeleteFileW(previous.c_str());
    if (source.empty() || !eyebot::clips::ordinary(source, false)) return;
    std::filesystem::copy_file(source, previewFile, std::filesystem::copy_options::none, error);
    if (!error) wrist.preview(*gameConsole, std::string("RobCoEyebot/") + previewFile.filename().string());
}

void closeDeviceSettings() {
    if(deviceEditor)editorCommand(fc_settings::EditorClose,deviceView);
    if(deviceSettings)settingsCommand(fc_settings::DeviceClose);
    poolPageOpen=equipmentOpen=false;
    visibleButtons=0;compactSettings=false;
    shownLayout={};
    equipmentTexture.clear();
    layoutTextureName.clear();
    deviceEditor=deviceSettings=false;
    wristSettingsInput=false;
    wristEyebotInput=false;
    deviceNavigation={};settingsAt=0;
}

std::atomic<bool> wristWorkshopActive{false};
void closeWrist(bool restoreCategory) {
    wristWorkshopActive=false;
    wristHardware.setSelected(false);
    fullFilthyPageState=0;
    fullPageGripArmed=0;pointedButton=0;visibleButtons=0;compactSettings=false;
    closeDeviceSettings();
    voiceOpen = settlementOpen = false;
    voice.cancel();
    wrist.close(*gameConsole);
    if(wrist.independent())eyebot::full_page::close();
    if(restoreCategory)eyebot::native_tabs::leaveWorn();
    eyebot::native_tabs::selection=eyebot::native_tabs::Selection::Stock;
    pilotActive = false;
    wristInputOpen = false;
    liveWristOpen = false;
    nativeWristReady = false;
    nativeWrist.reset();
    pilotFrameReady = false;
    nextPilotFrame = 0;
    if (!previewFile.empty()) DeleteFileW(previewFile.c_str());
    previewFile.clear();
}

namespace pages = eyebot::wrist_frame;

uint8_t filthyPages(){
    if(!settingsOnly)return 0;
    const auto pool=poolSettingsProvider();
    return (pool&&pool(fc_settings::DeviceRead,0,0,nullptr,0)?pages::bit(pages::Pool):0)|pages::bit(pages::General)|(controlsBound?pages::bit(pages::Eyebot):0)|
        (voiceReady?pages::bit(pages::Voice):0)|(settlementDesk(0)?pages::bit(pages::Workshop):0)|
        (equipment.present()?pages::bit(pages::Equipment):0);
}

pages::FilthyPage activeFilthyPage(){
    return settlementOpen?pages::Workshop:equipmentOpen?pages::Equipment:poolPageOpen?pages::Pool:voiceOpen?pages::Voice:deviceSettings?pages::General:pages::Eyebot;
}

void updateFullFilthyTabs(){
    if(settingsOnly&&!onArm){
        const auto available=filthyPages();
        const auto active=activeFilthyPage();
        fullFilthyPageState=uint16_t(available|(active<<8));
        wrist.filthyTabs(*gameConsole,active,available);
    }
}

void publishWristImage(const char* status, const char* help, const char* clip) {
    if (!onArm||deviceSettings||voiceOpen||settlementOpen) return;
    std::lock_guard<std::mutex> lock(wristImageMutex);
    wristImage.selection = wristSelection;
    wristImage.count = static_cast<uint32_t>(library.size());
    wristImage.confirm = wrist.confirming;
    wristImage.library = wrist.library;
    wristImage.piloting = !wrist.library && replay.state() == eyebot::State::Placed;
    wristImage.hasStill = wristImage.piloting ? pilotFrameReady : wristHasPhoto;
    wristImage.settings = false;
    wristImage.activeFilthyPage=pages::Eyebot;
    wristImage.availableFilthyPages=filthyPages();
    wristImage.nextFilthyPage = pages::nextPage(wristImage.activeFilthyPage, wristImage.availableFilthyPages);

    strncpy_s(wristImage.status, status, _TRUNCATE);
    strncpy_s(wristImage.hint, help, _TRUNCATE);
    strncpy_s(wristImage.clip, clip, _TRUNCATE);
    ++wristImage.generation;
    if (!qaDirectory.empty()) {
        std::ofstream proof(qaDirectory + "\\eyebot-arm-state.json");
        proof << "{\"generation\":" << wristImage.generation
              << ",\"state\":" << int(replay.state()) << ",\"library\":" << wristImage.library
              << ",\"pid\":" << GetCurrentProcessId() << ",\"selection\":" << wristSelection
              << ",\"count\":" << wristImage.count << ",\"still\":" << wristImage.hasStill
              << ",\"holsters\":" << wristImage.holsters << ",\"profile\":" << wristImage.profile
              << ",\"holsterRow\":" << wristImage.holsterRow << ",\"holsterState\":" << wristImage.holsterState << "}\n";
    }
}

void highlightWrist() {
    wrist.highlight(*gameConsole, wristSelection);
    if (onArm) {
        std::lock_guard<std::mutex> lock(wristImageMutex);
        wristImage.selection = wristSelection;
        wristImage.confirm = wrist.confirming;
        ++wristImage.generation;
    }
}

void refreshWrist() {
    if(deviceSettings)return;
    using eyebot::State;

    if (wrist.library) {
        if (wristSelection < eyebot::WristPage::Previous ||
            wristSelection > eyebot::WristPage::ConfirmTrash) wristSelection = eyebot::WristPage::Next;
        library = eyebot::clips::list(clipDirectory());
        if (libraryIndex >= library.size()) libraryIndex = library.empty() ? 0 : library.size() - 1;
        const auto label = library.empty() ? std::string("NO SAVED CLIPS") :
            "CLIP " + std::to_string(libraryIndex + 1) + " / " + std::to_string(library.size());
        const auto stem = library.empty() ? std::string() : library[libraryIndex].stem().string();
        wrist.label(*gameConsole, "Page/Status", label);
        wrist.label(*gameConsole, "Page/Controls", library.empty() ? "Saved recordings appear here." : stem);
        auto still = library.empty() ? std::filesystem::path{} : library[libraryIndex];
        if (!still.empty()) still.replace_extension(L".dds");
        showWristStill(still);
        publishWristImage(label.c_str(), "Saved recordings", stem.c_str());
        highlightWrist();
        return;
    }
    const auto state = replay.state();
    const char* status = "STOWED";
    const char* help = "Deploy to position the recorder.";
    switch (state) {
        case State::Placed:
            status = "POSITIONING";
            help = "Left stick moves. Right stick aims. RT rises. LT lowers.";
            break;
        case State::Parked: status = "PARKED"; help = "Fly / Aim repositions the Eyebot."; break;
        case State::Recording: status = "RECORDING"; help = "Close the wrist to play. Deploy / Recall ends the take."; break;
        case State::Review: status = movieSaved ? "MP4 SAVED" : "TAKE READY"; help = "Save MP4 or start a new take."; break;
        case State::Exporting: status = "SAVING MP4"; help = "The Eyebot is rendering the recording."; break;
        default: break;
    }
    wrist.label(*gameConsole, "Page/Status", status);
    wrist.label(*gameConsole, "Page/Controls", help);
    if (state == State::Off || wristSelection > eyebot::WristPage::Back) wristSelection = eyebot::WristPage::Deploy;
    publishWristImage(status, help);
    highlightWrist();
}

uint32_t floatBits(float value) {
    uint32_t bits;
    std::memcpy(&bits, &value, sizeof(bits));
    return bits;
}

void pilot(Form* device, Form* player, float seconds, uint64_t now) {
    if (!device || !player || !positioned || !scene::root(device) ||
        scene::cell(device) != scene::cell(player)) return;
    float axis[4]{};
    const auto freshStick = [now](int hand) {
        const auto sample = stickTime[hand].load();
        return sample && now - sample < 160;
    };
    const bool direct = freshStick(0) || freshStick(1);
    if (direct) {
        for (int hand = 0; hand != 2; ++hand) {
            if (!freshStick(hand)) continue;
            axis[hand*2] = stick[hand][0].load();
            axis[hand*2+1] = stick[hand][1].load();
        }
    } else {
        const auto values = call(wrist.input);
        Element numbers[4];
        if (values.type == 4 && arrays->size(values.value.array) == 4 &&
            arrays->elements(values.value.array, numbers, nullptr))
        for (unsigned i = 0; i != 4; ++i)
            if (numbers[i].type == 1 && std::isfinite(numbers[i].value.number))
                axis[i] = float(std::clamp(numbers[i].value.number, -1.0, 1.0));
    }
    auto p = scene::field<scene::Vec>(device, 0x30);
    const auto heading = scene::field<scene::Vec>(player, 0x24).z;
    const auto cast = [&](scene::Vec a, scene::Vec b) {
        Element value;
        scripts->call(wrist.moveClear, nullptr, nullptr, &value, 6, floatBits(a.x), floatBits(a.y),
            floatBits(a.z), floatBits(b.x), floatBits(b.y), floatBits(b.z));
        return value;
    };
    Element hit{};
    if (!eyebot::flight::advance(axis, liftControl.axis(now), heading, seconds,
                                 parkOrigin, p, flightYaw, flightPitch, cast, hit)) return;
    scripts->call(wrist.shift, nullptr, nullptr, nullptr, 6, device, floatBits(p.x), floatBits(p.y),
        floatBits(p.z), floatBits(flightYaw), floatBits(flightPitch));
    placeLens(device);
    if (!qaDirectory.empty()) {
        static uint64_t next = 0;
        if (now >= next) {
            next = now + 220;
            std::ofstream output(qaDirectory + "\\eyebot-wrist-drive.json");
            output << "{\"raw\":" << direct << ",\"axes\":["
                   << axis[0] << ',' << axis[1] << ',' << axis[2] << ',' << axis[3]
                   << "],\"hitType\":" << int(hit.type) << ",\"hit\":" << hit.value.number
                   << ",\"target\":[" << p.x << ',' << p.y << ',' << p.z
                   << "],\"yaw\":" << flightYaw << "}\n";
        }
    }
}

void dispatchWrist(eyebot::WristPage::Button);

void showSettings(const fc_settings::View& view,bool force=false) {
    fc_equipment::LayoutView layout{};
    if(equipmentOpen && view.page==fc_settings::WristEquipment &&
       (!equipment.layoutView(view,layout)||layout.count>8))layout={};
    fc_equipment::Controllers controllers{};
    bool mapping=view.page==fc_settings::WristControls && deviceEditor;
    if(mapping){
        const auto layer=GetModuleHandleW(L"SeatedInput.dll");
        auto read=layer?reinterpret_cast<fc_equipment::ReadControllers>(GetProcAddress(layer,"FC_ControllerLayoutView")):nullptr;
        mapping=read&&read(&controllers,sizeof(controllers));
    }
    const bool changed=std::memcmp(&layout,&shownLayout,sizeof(layout))!=0 ||
        std::memcmp(&controllers,&shownControllers,sizeof(controllers))!=0;
    if(!force&&!changed&&!std::memcmp(&view,&deviceView,sizeof(view)))return;
    if((layout.count||mapping) && (force||changed)){
        auto pixels=std::make_unique<eyebot::equipment::Picture>();
        controllerPicture=mapping && eyebot::device_art::controllers(controllers,pixels->data(),eyebot::native_theme::installed().colour());
        if(layout.count)eyebot::equipment::paintLayout(layout,*scripts,*pixels);
        if(onArm){
            std::lock_guard<std::mutex> lock(wristImageMutex);
            equipmentPhotograph=*pixels;
        }else layoutTextureName=equipmentTexture.publish(*pixels);
    }else if(!layout.count && !mapping && !layoutTextureName.empty()){
        equipmentTexture.clear();layoutTextureName.clear();
    }
    if(!mapping)controllerPicture=false;
    shownControllers=controllers;
    shownLayout=layout;
    deviceView=view;
    if(onArm){
        std::lock_guard<std::mutex> lock(wristImageMutex);
        wristImage.settings=true;
        wristImage.settingsView=view;
        wristImage.activeFilthyPage=activeFilthyPage();
        wristImage.availableFilthyPages=filthyPages();
        wristImage.nextFilthyPage = pages::nextPage(wristImage.activeFilthyPage, wristImage.availableFilthyPages);
        wristImage.piloting=0;
        wristImage.hasStill=layout.count!=0||controllerPicture;
        ++wristImage.generation;
    }else{
        wrist.settingsContent(*gameConsole,view,layoutTextureName);
        compactSettings=!layoutTextureName.empty();
        const int first=fc_settings::firstVisible(view);
        unsigned available=1u<<pages::BackButton;
        for(int i=0;i<fc_settings::DeviceRows&&i+first<int(view.count);++i)
            if(view.rows[first+i].kind!=fc_settings::Unavailable)available|=1u<<(pages::Row0+i);
        if(view.count>fc_settings::DeviceRows){
            if(first>0)available|=1u<<pages::LessButton;
            if(first+fc_settings::DeviceRows<int(view.count))available|=1u<<pages::MoreButton;
        }else if(view.selected<view.count&&view.rows[view.selected].kind==fc_settings::Number)
            available|=(1u<<pages::LessButton)|(1u<<pages::MoreButton);
        visibleButtons=available;
    }
    if(!qaDirectory.empty()){
        std::ofstream proof(qaDirectory+"\\wrist-settings-state.json");
        proof<<"{\"onArm\":"<<onArm<<",\"pid\":"<<GetCurrentProcessId()<<",\"availablePages\":"<<unsigned(filthyPages())<<",\"page\":"<<view.page<<",\"selected\":"<<view.selected
             <<",\"count\":"<<view.count<<",\"firstValue\":"<<std::quoted(view.rows[0].value)
             <<",\"note\":"<<std::quoted(view.note)<<",\"title\":"<<std::quoted(view.title)
             <<",\"nativeTheme\":"<<eyebot::native_theme::installed().available()<<",\"labels\":[";
        for(unsigned i=0;i<view.count;++i){
            if(i)proof<<',';
            proof<<std::quoted(view.rows[i].label);
        }
        proof<<"],\"controllerPicture\":"<<controllerPicture<<",\"layoutSlots\":"<<layout.count<<",\"layoutFocus\":"<<layout.focus<<",\"layoutForms\":[";
        for(unsigned i=0;i<layout.count;++i){if(i)proof<<',';proof<<layout.slots[i].form;}
        proof<<"],\"values\":[";
        for(unsigned i=0;i<view.count;++i){
            if(i)proof<<',';
            proof<<std::quoted(view.rows[i].value);
        }
        proof<<"]}\n";
    }
}

bool openDeviceSettings() {
    fc_settings::View view;
    if(!settingsCommand(fc_settings::DeviceOpen,0,0,&view))return false;
    deviceSettings=wristSettingsInput=true;
    deviceNavigation={};settingsAt=0;
    wrist.settingsDisplay(*gameConsole,true);
    showSettings(view,true);
    updateFullFilthyTabs();
    return true;
}

void activateVoice() {
#ifdef FC_VOICE_TEST
    const auto fixture = std::filesystem::path(qaDirectory) / "robco-voice-test.wav";
    voice.activate(!qaDirectory.empty() && std::filesystem::is_regular_file(fixture) ? fixture : std::filesystem::path{});
#else
    voice.activate();
#endif
}

bool showVoiceTab() {
    if (!voiceReady) return false;
    voiceOpen = true;
    wrist.settingsDisplay(*gameConsole,true);
    showSettings(voice.view(), true);
    updateFullFilthyTabs();
    return true;
}

void showSettlement() {
    fc_settings::View view;
    view.page=fc_settings::WristSettlement;
    auto row=[&](const char* label,const char* value){
        auto& r=view.rows[view.count++];strncpy_s(r.label,label,_TRUNCATE);strncpy_s(r.value,value,_TRUNCATE);
    };
    if(settlementView==SettlementView::Catalog){
        buildCatalog.paint(view);
    }else if(settlementView==SettlementView::Notices){
        auto read=fortMessages();
        fortNoticeToken=read?read(fort_messages::Read,0,0,fortTextPage,fortChoicePage,&view,sizeof view):0;
        if(!fortNoticeToken){strcpy_s(view.title,"MOD MESSAGES");row("No pending messages","BACK");}
    }else if(settlementView==SettlementView::DirectoryIndex){
        strcpy_s(view.title,"WHO'S WHO");
        row("Mojave groups","OPEN");row("Capital Wasteland groups","OPEN");row("All factions","OPEN");
        strcpy_s(view.note,"Regional shortlists or the full installed directory.");
    }else if(settlementView==SettlementView::Directory || settlementView==SettlementView::CompareList){
        strcpy_s(view.title,settlementView==SettlementView::Directory?"WHO'S WHO":"COMPARE WITH");
        for(unsigned i=0;i<crewPage.count;++i)row(crewPage.rows[i].name,"");
        if(!view.count)row("Directory unavailable","-");
        strncpy_s(view.note,crewPage.note,_TRUNCATE);
    }else if(settlementView==SettlementView::Faction){
        strncpy_s(view.title,crewPage.rows[0].name,_TRUNCATE);
        row("Representative",crewPage.rows[0].role);
        row("Compare factions","OPEN");
        row("Recruit to fort","10 FOOD");
        strncpy_s(view.note,crewPage.note,_TRUNCATE);
    }else if(settlementView==SettlementView::Comparison){
        strcpy_s(view.title,"FACTION RELATIONS");
        row(crewName.c_str(),crewPage.rows[0].role);
        row(otherFactionName.c_str(),crewPage.rows[1].role);
        strcpy_s(view.note,"Each row shows how that faction views the other.");
    }else if(settlementView==SettlementView::Home){
        strcpy_s(view.title,"ROBCO SETTLEMENTS");
        row("Build and place","OPEN");row("Settlers and jobs","OPEN");
        settlement_crew::Page policy;
        if(crewAction(settlement_crew::ReadPolicy,0,policy)){
            row("Who's Who","OPEN");row("Rival factions",policy.note);
        }
        if(fieldMode(0))row("Troop command","OPEN");
        if(crewAction(settlement_crew::ReadFreeBuild,0,policy))row("Free building",policy.note);
        row("Browse pieces","OPEN");
        if(auto notices=fortMessages();notices&&notices(fort_messages::Present,0,0,0,0,nullptr,0))
            row("Mod messages",std::to_string(notices(fort_messages::Count,0,0,0,0,nullptr,0)).c_str());
        strcpy_s(view.note,onArm?"Right stick click selects a row. A opens it.":"Choose Build and place to return to the world.");
    }else if(settlementView==SettlementView::Troops){
        strcpy_s(view.title,"TROOP COMMAND");
        for(unsigned i=0;i<crewPage.count;++i)row(crewPage.rows[i].name,crewPage.rows[i].role);
        if(crewOffset)row("Previous page","OPEN");
        if(crewOffset+crewPage.count<crewPage.total)row("Next page","OPEN");
        if(!view.count){row("Command radio","OPEN");row("Move a marker","OPEN");}
        strncpy_s(view.note,crewPage.count?crewPage.note:"The probe points and issues orders in the world.",_TRUNCATE);
    }else if(settlementView==SettlementView::Orders){
        strncpy_s(view.title,crewName.c_str(),_TRUNCATE);
        if(companyGuard){row("Place guard post","AIM");row("Resume fort duty","ORDER");}
        else {row("Assign a job","OPEN");row("Call over","ORDER");row("Send to work","ORDER");}
        strncpy_s(view.note,crewMessage.c_str(),_TRUNCATE);
    }else{
        strcpy_s(view.title,settlementView==SettlementView::Roster?"SETTLERS":"AVAILABLE JOBS");
        for(unsigned i=0;i<crewPage.count;++i)row(crewPage.rows[i].name,crewPage.rows[i].role);
        if(crewOffset)row("Previous page","OPEN");
        if(crewOffset+crewPage.count<crewPage.total)row("Next page","OPEN");
        if(!view.count)row("No entries","-");
        strncpy_s(view.note,crewPage.note,_TRUNCATE);
    }
    view.selected=std::min(settlementRow,view.count-1);
    showSettings(view);
}
bool openSettlement() {
    if(!settlementDesk(0))return false;
    settlementOpen=true;settlementRow=0;settlementView=SettlementView::Home;
    wrist.settingsDisplay(*gameConsole,true);showSettlement();return true;
}
void readCrew(SettlementView view, unsigned offset=0) {
    settlementView=view;settlementRow=0;crewOffset=offset;crewPage={};
    const auto action=view==SettlementView::Roster?settlement_crew::Roster:
        view==SettlementView::Jobs?settlement_crew::Jobs:settlement_crew::Directory;
    if(!crewAction(action,offset,crewPage))
        strcpy_s(crewPage.note,"Forts roster unavailable.");
    showSettlement();
}
void browseFactions(SettlementView view){
    settlementView=view;settlementRow=0;crewPage={};
    const auto range=directoryTrail.empty()?settlement_crew::Range{}:directoryTrail.back();
    const auto list=directoryRegion==1?settlement_crew::MojaveDirectory:
        directoryRegion==2?settlement_crew::CapitalDirectory:settlement_crew::Directory;
    if(!crewRequest(list,range.count,range.first,crewPage))
        strcpy_s(crewPage.note,"Faction directory unavailable.");
    showSettlement();
}
void backSettlement() {
    if(settlementView==SettlementView::Catalog && buildCatalog.back()){settlementRow=0;showSettlement();return;}
    if((settlementView==SettlementView::Directory||settlementView==SettlementView::CompareList)&&!directoryTrail.empty()){
        directoryTrail.pop_back();browseFactions(settlementView);return;
    }
    if(settlementView==SettlementView::Directory){settlementView=SettlementView::DirectoryIndex;settlementRow=0;showSettlement();return;}
    if(settlementView==SettlementView::Faction){directoryTrail=originTrail;directoryRegion=originRegion;browseFactions(SettlementView::Directory);return;}
    if(settlementView==SettlementView::CompareList || settlementView==SettlementView::Comparison){
        settlementView=SettlementView::Faction;settlementRow=0;crewResident=originSlot;
        crewAction(settlement_crew::InspectFaction,0,crewPage);showSettlement();return;
    }
    if(settlementView==SettlementView::Home){if(!settingsOnly)closeWrist();return;}
    if(settlementView==SettlementView::Orders){readCrew(SettlementView::Roster);return;}
    if(settlementView==SettlementView::Troops){fieldMode(6);settlementView=SettlementView::Home;settlementRow=0;showSettlement();return;}
    settlementView=settlementView==SettlementView::Jobs?SettlementView::Orders:SettlementView::Home;
    settlementRow=0;showSettlement();
}
void enterFieldFromBuild(){
    settlementView=SettlementView::Home;
    settlementRow=0;
    if(onArm){showSettlement();return;}
    if(!wrist.open||!fullPipboyReady||!wrist.selected)return;
    returnToBuild=true;
    // Close the actual Pip-Boy while FILTHY still owns its surface. Hiding the
    // page first exposes whichever stock menu originally hosted that surface.
    gameConsole->run("CloseActiveMenu 1",nullptr);
    wrist.nextTabCheck=0;
}
void activateSettlement(unsigned row) {
    if(settlementView==SettlementView::Notices){
        if(row>=deviceView.count)return;
        if(!fortNoticeToken){backSettlement();return;}
        const auto& line=deviceView.rows[row];
        if(line.kind==fc_settings::Number){
            int current=0,total=0;
            if(sscanf_s(line.value,"%d / %d",&current,&total)==2&&total>0){
                if(!strcmp(line.label,"Message text"))fortTextPage=current%total;
                if(!strcmp(line.label,"More choices"))fortChoicePage=current%total;
            }
        }else if(auto submit=fortMessages()){
            submit(fort_messages::Choose,fortNoticeToken,fortChoicePage*6+int(row),0,0,nullptr,0);
            fortTextPage=fortChoicePage=0;
        }
        settlementRow=0;showSettlement();return;
    }
    if(settlementView==SettlementView::Catalog){buildCatalog.choose(row);settlementRow=0;showSettlement();return;}
    if(settlementView==SettlementView::DirectoryIndex){
        if(row>2)return;
        directoryRegion=row<2?int(row+1):0;directoryTrail.clear();browseFactions(SettlementView::Directory);return;
    }
    if(settlementView==SettlementView::Comparison){backSettlement();return;}
    if(settlementView==SettlementView::Faction){
        if(row==1){directoryRegion=0;directoryTrail.clear();browseFactions(SettlementView::CompareList);return;}
        if(row==2){
            settlement_crew::Page outcome;
            if(crewAction(settlement_crew::RecruitFaction,0,outcome))strcpy_s(crewPage.note,outcome.note);
            showSettlement();
        }
        return;
    }
    if(settlementView==SettlementView::Directory || settlementView==SettlementView::CompareList){
        if(row<crewPage.count){
            const auto entry=crewPage.rows[row];
            const auto width=settlement_crew::directoryWidth(crewPage.total);
            if(width>1){
                directoryTrail.push_back({entry.id,std::min(width,crewPage.total-width*row)});
                browseFactions(settlementView);return;
            }
            if(settlementView==SettlementView::CompareList){
                otherFactionName=entry.name;
                if(!crewAction(settlement_crew::CompareFactions,entry.id,crewPage))return;
                settlementView=SettlementView::Comparison;
            }else{
                originSlot=crewResident=entry.id;originTrail=directoryTrail;originRegion=directoryRegion;crewName=entry.name;
                if(!crewAction(settlement_crew::InspectFaction,0,crewPage))return;
                settlementView=SettlementView::Faction;
            }
            settlementRow=0;showSettlement();return;
        }
        return;
    }
    if(settlementView==SettlementView::Troops){
        settlement_crew::Page result;
        if(!crewPage.count)fieldMode(row==1?4:3);
        else if(row<crewPage.count){fieldMenu(crewOffset,int(crewPage.rows[row].id),result);settlementRow=0;crewOffset=0;}
        else if(row==crewPage.count&&crewOffset){crewOffset-=settlement_crew::Rows;settlementRow=0;}
        else if(crewOffset+crewPage.count<crewPage.total){crewOffset+=settlement_crew::Rows;settlementRow=0;}
        fieldMenu(crewOffset,-1,crewPage);showSettlement();return;
    }
    if(settlementView==SettlementView::Home){
        if(row==0&&settlementDesk(1)){fieldMode(1);enterFieldFromBuild();}
        else if(row==1)readCrew(SettlementView::Roster);
        else if(row>=2 && row<deviceView.count){
            const std::string label=deviceView.rows[row].label;
            if(label=="Mod messages"){settlementView=SettlementView::Notices;settlementRow=0;fortTextPage=fortChoicePage=0;showSettlement();return;}
            if(label=="Browse pieces"){
                if(fieldMode(1)&&buildCatalog.open(*scripts)){
                    settlementView=SettlementView::Catalog;settlementRow=0;showSettlement();
                }else{
                    auto view=deviceView;
                    strcpy_s(view.note,"Finish or cancel the current placement before browsing.");
                    showSettings(view);
                }
                return;
            }
            if(label=="Free building"){settlement_crew::Page outcome;crewAction(settlement_crew::ToggleFreeBuild,0,outcome);showSettlement();return;}
            if(label=="Who's Who"){settlementView=SettlementView::DirectoryIndex;settlementRow=0;showSettlement();return;}
            if(label=="Rival factions"){settlement_crew::Page outcome;crewAction(settlement_crew::TogglePolicy,0,outcome);showSettlement();return;}
            if(label=="Troop command"&&fieldMode(2)){settlementView=SettlementView::Troops;crewPage={};crewOffset=settlementRow=0;showSettlement();}
        }
        return;
    }
    if(settlementView==SettlementView::Orders){
        if(companyGuard){
            settlement_crew::Page result;
            const bool accepted=crewAction(row==0?settlement_crew::BeginGuardPost:settlement_crew::ReleaseGuardPost,0,result);
            if(row==0&&accepted){
                enterFieldFromBuild();
                return;
            }
            crewMessage=accepted?result.note:"No active guard post.";showSettlement();return;
        }
        if(row==0){readCrew(SettlementView::Jobs);return;}
        if(row>2)return;
        settlement_crew::Page outcome;
        crewMessage=crewAction(row==1?settlement_crew::CallOver:settlement_crew::SendToWork,0,outcome)?outcome.note:"Order unavailable.";
        showSettlement();return;
    }
    if(row<crewPage.count){
        if(settlementView==SettlementView::Roster){
            crewResident=crewPage.rows[row].id;crewName=crewPage.rows[row].name;crewMessage.clear();
            const std::string role=crewPage.rows[row].role;companyGuard=role=="Fort guard"||role=="On guard post";
        }else{
            settlement_crew::Page outcome;
            crewMessage=crewAction(settlement_crew::Assign,crewPage.rows[row].id,outcome)?outcome.note:"Order unavailable.";
        }
        settlementView=SettlementView::Orders;settlementRow=0;showSettlement();return;
    }
    if(row==crewPage.count&&crewOffset){readCrew(settlementView,crewOffset-settlement_crew::Rows);return;}
    if(crewOffset+crewPage.count<crewPage.total)readCrew(settlementView,crewOffset+settlement_crew::Rows);
}

void selectFilthyPage(pages::FilthyPage page){
    if(deviceEditor||page==activeFilthyPage()||!(filthyPages()&pages::bit(page)))return;
    closeDeviceSettings();
    if(voiceOpen)voice.cancel();
    if(settlementOpen&&settlementView==SettlementView::Troops)fieldMode(6);
    settlementOpen=false;
    voiceOpen=false;
    pilotActive=false;
    wrist.showLibrary(*gameConsole,false);
    if(page==pages::Workshop){
        if(!openSettlement())openDeviceSettings();
        updateFullFilthyTabs();
    }else if(page==pages::Voice){
        showVoiceTab();
    }else if(page==pages::General||page==pages::Pool||page==pages::Equipment){
        equipmentOpen=page==pages::Equipment;
        poolPageOpen=page==pages::Pool;
        if(!openDeviceSettings()&&(poolPageOpen||equipmentOpen)){
            poolPageOpen=equipmentOpen=false;
            openDeviceSettings();
        }
    }else{
        wristEyebotInput=true;
        wrist.settingsDisplay(*gameConsole,false);
        refreshWrist();
        updateFullFilthyTabs();
    }
}

void cycleWristTabs(int direction=1){
    if(settingsOnly){
        const auto next = pages::nextPage(activeFilthyPage(), filthyPages(),direction);
        if (next != pages::NoPage) selectFilthyPage(next);
        return;
    }
    if (settlementOpen) {
        settlementOpen=false;
    } else if (voiceOpen) {
        voiceOpen = false;
        voice.cancel();
        if(openSettlement())return;
    } else if(deviceSettings){
        closeDeviceSettings();
        if (showVoiceTab()) return;
    }else if(openDeviceSettings())return;
    wrist.settingsDisplay(*gameConsole,false);refreshWrist();
}

void deviceAction(unsigned command,int row=0,int direction=0){
    // Back navigates within a FILTHY page. Returning false from the legacy
    // settings home used to close the category and reveal native DATA.
    if(settingsOnly&&command==fc_settings::DeviceBack&&
       (deviceView.page==fc_settings::WristHome||poolPageOpen))return;
    fc_settings::View view;
    if(!settingsCommand(command,row,direction,&view)){
        // Optional pages can disappear during play (for example when a rack
        // ends). Return to General within the same active device category.
        if(settingsOnly){if(poolPageOpen||equipmentOpen)selectFilthyPage(pages::General);}
        else closeWrist();
        return;
    }
    if(view.page==fc_settings::WristControls&&!deviceEditor){
        deviceEditor=editorCommand(fc_settings::EditorOpen,view);
        wristSettingsInput=!deviceEditor;
        if(!deviceEditor){settingsCommand(fc_settings::DeviceBack,0,0,&view);strcpy_s(view.note,"Controller editor unavailable.");}
    }
    showSettings(view);
}

int browseDirection(uint64_t now,int& adjust){
    const auto at=settingsAt.load();
    const bool current=at&&(at>=now||now-at<200);
    const float x=current?settingsX.load():0.f;
    const float y=current?settingsY.load():0.f;
    const int old=deviceNavigation.selected;
    adjust=deviceNavigation.step(x,y,now,2);
    return old==deviceNavigation.selected?0:(y>0?-1:1);
}

void tickDeviceSettings(unsigned edges,uint64_t now){
    if(deviceEditor){
        fc_settings::View view;
        if(editorCommand(fc_settings::EditorRead,view))showSettings(view);
        else {deviceEditor=false;wristSettingsInput=true;deviceAction(fc_settings::DeviceBack);}
        return;
    }
    if(edges&4){deviceAction(fc_settings::DeviceBack);return;}
    if(edges&1){deviceAction(fc_settings::DeviceMove,0,1);return;}
    if(edges&2){deviceAction(fc_settings::DeviceSelect,deviceView.selected);return;}
    int adjust=0;
    const int move=browseDirection(now,adjust);
    if(move)deviceAction(fc_settings::DeviceMove,0,move);
    else if(adjust)deviceAction(fc_settings::DeviceAdjust,deviceView.selected,adjust);
    else if(equipmentOpen && shownLayout.kind==fc_equipment::Kind::Holsters){
        static uint64_t nextRefresh=0;
        if(now>=nextRefresh){nextRefresh=now+350;showSettings(deviceView);}
    }
}

bool showWorkshopOnWrist(){
    const auto field=GetModuleHandleW(L"FilthyCasualEnclave.dll");
    using CurrentGuard=bool(__cdecl*)(char*,unsigned);
    const auto guard=field?reinterpret_cast<CurrentGuard>(GetProcAddress(field,"FC_CompanyCurrentGuard")):nullptr;
    auto duty=deviceView;
    char guardName[64]{};
    if(guard&&guard(guardName,sizeof guardName)){
        sprintf_s(duty.note,"%s. Grip to aim, release and tap right trigger to post.",guardName);
        showSettings(duty);return true;
    }
    const auto module=GetModuleHandleW(L"FilthyCasualSettlements.dll");
    const auto read=module?reinterpret_cast<settlement_ui::Snapshot>(GetProcAddress(module,"FC_SettlementUI")):nullptr;
    settlement_ui::State state;
    if(!read||!read(&state,sizeof(state))||state.mode!=1)return false;
    _snprintf_s(duty.note,_TRUNCATE,"%s / %s. %s",state.category,state.item,
                state.stage==5?"Right trigger confirms. B goes back.":"Lower the wrist to aim.");
    showSettings(duty);
    return true;
}

// Record ownership changes in isolated QA, including frames when the wrist is
// lowered. Page content files alone can outlive the menu that produced them.
struct WristFocusProof {
    ~WristFocusProof(){
        if(qaDirectory.empty())return;
        static unsigned previous=~0u;
        const auto owner=unsigned(eyebot::native_tabs::selection.load());
        const auto page=wrist.open?unsigned(activeFilthyPage()):0u;
        const auto menu=wrist.selected?unsigned(wrist.selected->id):0u;
        const unsigned state=(menu<<12)|(page<<6)|(owner<<4)|(onArm?8u:0u)|
            (wrist.open?4u:0u)|(wristInputOpen?2u:0u)|(nativeWristReady?1u:0u);
        if(state==previous)return;
        previous=state;
        std::ofstream out(qaDirectory+"\\filthy-focus.csv",std::ios::app);
        out<<GetCurrentProcessId()<<','<<GetTickCount64()<<','<<owner<<','<<page<<','<<menu<<','
           <<wrist.open<<','<<onArm<<','<<wristInputOpen.load()<<','<<nativeWristReady.load()<<'\n';
    }
};

void tickWrist() {
    if (!wristReady) return;
    const WristFocusProof proof;
    const auto now = GetTickCount64();
    const eyebot::native_tabs::PendingTransition transition;
    const int tabRequest=transition.value;
    if (tabRequest || now >= wrist.nextTabCheck) {
        wrist.nextTabCheck = now + 100;
        eyebot::native_tabs::restoreActiveData();
        const auto active = call(wrist.menuMode);
        auto menu = wrist.stockMenu(active.type == 1 ? int(active.value.number) : 0);
        const auto seen = liveWristAt.load();
        const auto focused = focusedWristAt.load();
        const bool hasWrist = settingsOnly && eyebot::native_tabs::installed
            ? eyebot::native_tabs::wornPipboyActive()
            : seen && now>=seen && now-seen<275;
        const bool live = !menu && active.type == 1 && active.value.number == 0 && hasWrist;
        const auto core=GetModuleHandleW(L"SeatedControls.dll");
        const bool hasSettings=core&&GetProcAddress(core,"FC_WristSettings");
        eyebot::native_tabs::wornAvailable=hasSettings;
        // A lowered wrist, the wrist HUD and other game menus hide the screen
        // without changing the selected worn category or its current page.
        if(settingsOnly&&onArm&&wrist.open&&!menu){
            const bool navigating = live && eyebot::native_tabs::wornNavigationActive();
            if (wristInputOpen.exchange(navigating) != navigating) {
                wristSettingsInput = navigating && deviceSettings && !deviceEditor;
                if(deviceSettings)settingsCommand(fc_settings::DevicePresent,0,navigating?1:0);
                if(!navigating){wristInputEdges=0;settingsAt=0;settingsStickDrain=false;}
            }
            if(liveWristOpen.exchange(live)!=live){
                if(!live){
                    nativeWristReady=false;
                    nativeWrist.reset();
                    pilotActive=false;
                    wrist.lastTick=0;
                    wristInputEdges=0;
                }
                if(!qaDirectory.empty()){
                    std::ofstream trace(qaDirectory+"\\filthy-wrist-visibility.txt",std::ios::app);
                    trace<<(live?"resumed":"suspended")<<'\n';
                }
            }
            if(!live){
                wristInputAvailable=false;
                return;
            }
        }
        if (menu != wrist.selected || live != onArm) {
            filthyPointer=0;
            closeWrist();
        }
        onArm = live;
        wrist.selected = menu;
        const auto stockTab = menu && menu->stock ? call(menu->stock) : Element{};
        const bool stock = menu && stockTab.type == 1 && stockTab.value.number == 1;
        bool menuReady=wrist.independent();
        if (stock) {
            auto exists = call(menu->installed);
            if (exists.type == 1 && exists.value.number == 0) {
                call(menu->add);
                exists=call(menu->installed);
            }
            auto button=call(menu->hasButton);
            if(button.type==1&&button.value.number==0){
                call(menu->addButton);
                button=call(menu->hasButton);
            }
            menuReady=exists.type==1 && exists.value.number==1 && button.type==1 && button.value.number==1;
            if (menuReady && !wristBound) wristBound = wrist.bindButtons(*scripts);
        }
        fullPipboyReady = menuReady;
        eyebot::native_tabs::fullMenu=menuReady?menu->id:0;

        wrist.available = menuReady || live;
        wristInputAvailable = menuReady || (live && (wrist.open || (focused && now-focused < 220)));
        if (!wrist.available && wrist.open) closeWrist();
        else if(menu&&wrist.open)wrist.maintainStockVisibility(*gameConsole);
    }
    if(settingsOnly&&onArm&&wrist.open&&!liveWristOpen)return;
    using Button = eyebot::WristPage;
    auto edges = wristInputEdges.exchange(0);
    voice.poll(voiceOpen && wrist.open);
    if(tabRequest>=eyebot::native_tabs::exitWornBase){
        const int fullId=fullPipboyReady&&wrist.selected?wrist.selected->id:0;
        if(wrist.independent()){
            eyebot::native_tabs::finishExit(tabRequest,false,fullId);
            closeWrist(false);
        }else{
            if(wrist.open)closeWrist();
            eyebot::native_tabs::finishExit(tabRequest,onArm,fullId);
        }
        wrist.nextTabCheck=0;
        return;
    }
    if((tabRequest==eyebot::native_tabs::changeWornPage||tabRequest==eyebot::native_tabs::previousWornPage)&&onArm&&wrist.open){
        if(!deviceEditor)cycleWristTabs(tabRequest==eyebot::native_tabs::previousWornPage?-1:1);
        edges=0;
    }
    if(wrist.available&&!wrist.open&&
        ((tabRequest==eyebot::native_tabs::enterWorn&&onArm)||
         (tabRequest==eyebot::native_tabs::enterFull&&fullPipboyReady)))
        dispatchWrist(Button::Tab);
    if ((edges & SelectFilthyTab) && fullPipboyReady && !wrist.open) dispatchWrist(Button::Tab);
    if(fullPipboyReady&&wrist.open&&!onArm){
        const auto picked=pointedButton.exchange(0);
        if(picked)dispatchWrist(static_cast<Button::Button>(picked));
        if((edges&PreviousFullPage)&&!deviceEditor)cycleWristTabs(-1);
        if((edges&NextFullPage)&&!deviceEditor)cycleWristTabs(1);
        switch(edges&SelectFilthyPages){
            case SelectGeneral: selectFilthyPage(pages::General);break;
            case SelectEyebot: selectFilthyPage(pages::Eyebot);break;
            case SelectVoice: selectFilthyPage(pages::Voice);break;
            case SelectPool: selectFilthyPage(pages::Pool);break;
            case SelectWorkshop: selectFilthyPage(pages::Workshop);break;
            case SelectEquipment: selectFilthyPage(pages::Equipment);break;
            default: break;
        }
    }
    if (wrist.available && (wristBound || onArm || settingsOnly)) {
        if ((edges & 8) && wrist.open && !deviceEditor && !pilotActive && !wrist.library && (!settingsOnly || !onArm)) {
            cycleWristTabs();
        } else if (settlementOpen && wrist.open) {
            if (edges & 4) backSettlement();
            else {
                if(edges & 1) {settlementRow=(settlementRow+1)%deviceView.count;showSettlement();}
                if(edges & 2) activateSettlement(settlementRow);
                if(settlementOpen&&settlementView==SettlementView::Troops){
                    if(fieldMode(5)){
                        if(onArm){returnToBuild=true;closeWrist();}
                        else enterFieldFromBuild();
                    }
                    else {fieldMenu(crewOffset,-1,crewPage);showSettlement();}
                }
            }
        } else if (voiceOpen && wrist.open) {
            if (edges & 4){dispatchWrist(Button::Back);}
            else {
                if (edges & 2) activateVoice();
                showSettings(voice.view());
            }
        } else if(deviceSettings&&wrist.open) {
            tickDeviceSettings(edges,now);
        } else {
        int adjust=0;
        const int move=wrist.open&&!pilotActive?browseDirection(now,adjust):0;
        if ((edges & 1)||move) {
            if (!wrist.open) dispatchWrist(Button::Tab);
            else if (pilotActive) dispatchWrist(Button::Stay);
            else if (onArm && replay.state() == eyebot::State::Off) wristSelection = Button::Deploy;
            else {
                const int first=wrist.library?Button::Previous:Button::Deploy;
                const int last=wrist.library?(wrist.confirming?Button::ConfirmTrash:Button::Trash):Button::Back;
                const int count=last-first+1;
                wristSelection=first+(std::clamp(wristSelection,first,last)-first+(move<0?-1:1)+count)%count;
                highlightWrist();
            }
        }
        if (edges & 2 && wrist.open) dispatchWrist(static_cast<Button::Button>(
            pilotActive ? Button::Stay : wristSelection));
        if (edges & 4 && wrist.open) dispatchWrist(Button::Back);
        if (edges & 8 && wrist.open && !wrist.library) dispatchWrist(Button::Snap);
        }
    }
    // Only the selected FILTHY Workshop page owns its native menu surface.
    // Raising the wrist to Stats, Data or another FILTHY page must leave it alone.
    const bool buildingOnWrist=settingsOnly&&wrist.open&&settlementOpen&&settlementView==SettlementView::Home&&fieldMode(7);
    if(buildingOnWrist&&!onArm)showWorkshopOnWrist();
    if(wristWorkshopActive.exchange(buildingOnWrist)&&!buildingOnWrist&&settlementOpen&&settlementView==SettlementView::Home)showSettlement();
    if (!settingsOnly && !wrist.open && !wristDeployPending && !buildingOnWrist&&!nativeWristReady.load()) return;
    auto player = form(call(getPlayer));
    if (settingsOnly && eyebot::native_tabs::installed)
        wristHardware.update(*scripts,scene::root(player),onArm&&wrist.open,now);
    if (!wrist.open && !wristDeployPending && !buildingOnWrist&&!nativeWristReady.load()) return;
    auto device = controlsBound ? form(call(controls.device)) : nullptr;
    if (wristDeployPending && device && player && scene::root(device) &&
        scene::cell(device) == scene::cell(player)) {
        wristDeployPending = false;
        if (placeLens(device)) {
            parkOrigin = scene::field<scene::Vec>(device, 0x30);
            flightYaw = scene::field<scene::Vec>(device, 0x24).z * 57.2957795f;
            flightPitch = 0.f;
            positioned = true;
            if (wrist.open&&!deviceSettings) refreshWrist();
        }
    }
    const bool move = wrist.open && !deviceSettings && !voiceOpen && !settlementOpen && !wrist.library && replay.state() == eyebot::State::Placed && positioned;
    pilotActive = move;
    if (move && wrist.lastTick) {
        const float seconds = std::min(float(now - wrist.lastTick) * .001f, .06f);
        if (seconds > 0.f) pilot(device, player, seconds, now);
    }
    if (move && onArm && !snapshot.waiting()) refreshPilotCamera(device, player, now);
    const auto seen=liveWristAt.load();
    const bool workshop=buildingOnWrist&&onArm&&seen&&now-seen<220&&showWorkshopOnWrist();
    if ((onArm && wrist.open)||workshop) {
        std::lock_guard<std::mutex> lock(wristImageMutex);
        if(workshop){wristImage.settings=true;wristImage.settingsView=deviceView;if(!wristImage.generation)++wristImage.generation;}
        const bool useNativePage=settingsOnly && eyebot::native_tabs::installed &&
            eyebot::native_tabs::wornCategory()==eyebot::native_tabs::filthyWornCategory;
        if(!useNativePage){
            const auto& source=devicePicture();
            const bool ready = nativeWrist.update(scene::root(player), wristImage, source.data());

            if (nativeWristReady.exchange(ready) != ready && !qaDirectory.empty()) {

                std::ofstream trace(qaDirectory + "\\eyebot-native-crt-state.txt", std::ios::app);

                trace << "pipboyscreen:0 " << (ready ? "bound" : "unavailable") << '\n';

            }
        }
    }
    if (!move) {
        pilotFrameReady = false;
        nextPilotFrame = 0;
    }
    wrist.lastTick = move ? now : 0;
}

void dispatchWrist(eyebot::WristPage::Button command) {
    using B = eyebot::WristPage;
    using eyebot::State;
    if (command == B::Tab) {
        if (wrist.open) { if(!settingsOnly)closeWrist(); }
        else {
            if(settingsOnly && !onArm){
                const bool entered=wrist.selected && eyebot::full_page::open(wrist.selected->id,eyebot::native_tabs::removePriorCategory);
                if(!qaDirectory.empty()){
                    std::ofstream trace(qaDirectory+"/filthy-native-entry.txt",std::ios::app);
                    trace<<entered<<" root "<<eyebot::full_page::root<<" instance "<<eyebot::full_page::instance<<std::endl;
                }
                if(!entered)return;
                eyebot::native_tabs::publishFullCategory();
                wrist.selected=&wrist.custom;
                eyebot::native_tabs::fullMenu=eyebot::full_page::id;
            }
            if(settingsOnly)eyebot::native_tabs::selection=onArm?
                eyebot::native_tabs::Selection::WornFilthy:eyebot::native_tabs::Selection::FullFilthy;
            wrist.display(*gameConsole, true);
            wristInputOpen = !onArm || !settingsOnly || eyebot::native_tabs::wornNavigationActive();
            liveWristOpen = onArm;
            wristSelection = B::Deploy;
            wrist.showLibrary(*gameConsole, false);
            if (!openDeviceSettings()) {
                if (settingsOnly) closeWrist();
                else refreshWrist();
            }else if(returnToBuild){
                returnToBuild=false;
                selectFilthyPage(pages::Workshop);
            }
        }
        return;
    }
    if (!wrist.open || (settingsOnly && !controlsBound && !deviceSettings && !deviceEditor && !voiceOpen && !settlementOpen && !poolPageOpen)) return;
    if(settingsOnly&&command>=B::GeneralSubtab&&command<=B::EquipmentSubtab){
        selectFilthyPage(static_cast<pages::FilthyPage>(pages::General+command-B::GeneralSubtab));
        return;
    }
    if(command==B::SettingsCycle||command==B::SettingsTab){if(!deviceEditor)cycleWristTabs();return;}
    if(settlementOpen){
        if(command==B::SettingsBack||command==B::Back)backSettlement();
        else if((command==B::SettingsLess||command==B::SettingsMore)&&deviceView.count>fc_settings::DeviceRows){
            const int step=command==B::SettingsMore?fc_settings::DeviceRows:-fc_settings::DeviceRows;
            settlementRow=std::clamp(int(deviceView.selected)+step,0,int(deviceView.count)-1);
            showSettlement();
        }
        else if(command>=B::SettingsRow0&&command<=B::SettingsLast){
            const unsigned row=fc_settings::firstVisible(deviceView)+command-B::SettingsRow0;
            if(row<deviceView.count)activateSettlement(row);
        }
        return;
    }
    if (voiceOpen) {
        if (command == B::SettingsBack || command == B::Back){
            if(settingsOnly)selectFilthyPage(pages::General);
            else closeWrist();
        }
        else if (command == B::SettingsRow0) { activateVoice(); showSettings(voice.view()); }
        return;
    }
    if(deviceEditor){
        if(command==B::SettingsBack)editorCommand(fc_settings::EditorBack,deviceView);
        else if(command>=B::SettingsRow0&&command<=B::SettingsLast)
            editorCommand(fc_settings::EditorChoose,deviceView,int(command)-B::SettingsRow0);
        return;
    }
    if(deviceSettings){
        if(command==B::SettingsBack)deviceAction(fc_settings::DeviceBack);
        else if((command==B::SettingsLess||command==B::SettingsMore)&&deviceView.count>fc_settings::DeviceRows)
        {
            const int direction=command==B::SettingsMore?1:-1;
            const int dest=std::clamp(int(deviceView.selected)+direction*fc_settings::DeviceRows,0,int(deviceView.count)-1);
            fc_settings::View next=deviceView;
            for(int i=0;i<std::abs(dest-int(deviceView.selected));++i)
                if(!settingsCommand(fc_settings::DeviceMove,0,direction,&next))break;
            showSettings(next);
        }
        else if(command==B::SettingsLess||command==B::SettingsMore)
            deviceAction(fc_settings::DeviceAdjust,deviceView.selected,command==B::SettingsMore?1:-1);
        else if(command>=B::SettingsRow0&&command<=B::SettingsLast){
            const int first=fc_settings::firstVisible(deviceView);
            const int row=first+command-B::SettingsRow0;
            if(row<int(deviceView.count))deviceAction(fc_settings::DeviceSelect,row);
        }
        return;
    }
    if(!controlsBound)return;
    if (command == B::Back) {
        if (wrist.library && wrist.confirming) {
            wrist.showConfirmation(*gameConsole, false);
            wristSelection = B::Trash;
        } else if (wrist.library) {
            wrist.showLibrary(*gameConsole, false);
            showWristStill({});
        } else if(pilotActive){dispatchWrist(B::Stay);return;}
        else if(settingsOnly){selectFilthyPage(pages::General);return;}
        else closeWrist();
    } else if (wrist.library) {
        if ((command == B::Previous || command == B::Next) && !library.empty()) {
            libraryIndex = (libraryIndex + (command == B::Next ? 1 : library.size()-1)) % library.size();
            wrist.showConfirmation(*gameConsole, false);
        } else if (command == B::Trash && !library.empty()) {
            wrist.showConfirmation(*gameConsole, true);
            wristSelection = B::ConfirmTrash;
            highlightWrist();
            return;
        } else if (command == B::ConfirmTrash && wrist.confirming && !library.empty()) {
            const auto selected = library[libraryIndex];
            if (eyebot::clips::trash(selected) && moviePath == selected && movieSaved) {
                movieSaved = false;
                replay.discard();
            }
            wrist.showConfirmation(*gameConsole, false);
        }
    } else {
        auto device = form(call(controls.device));
        if (!device) return;
        const auto state = replay.state();
        switch (command) {
            case B::Deploy:
                if (state == State::Recording) replay.recall();
                if (state == State::Off) {
                    scripts->call(controls.moveBeside, nullptr, nullptr, nullptr, 1, device);
                    wristDeployPending = true;
                }
                break;
            case B::Stay:
                if (placeLens(device)) replay.park();
                break;
            case B::Snap: takeSnapshot(device); break;
            case B::Record:
                if (state == State::Review && movieSaved) replay.discard();
                startRecording(device);
                break;
            case B::Save:
                if (!movieSaved) saveRecording(device);
                break;
            case B::Clips:
                libraryIndex = 0;
                wrist.showLibrary(*gameConsole, true);
                break;
            case B::Navigate:
                if (state == State::Review && movieSaved) replay.discard();
                if (replay.state() == State::Parked) placeLens(device);
                break;
            case B::Stow:
                if (state == State::Review && movieSaved) replay.discard();
                if (replay.state() == State::Placed || replay.state() == State::Parked) {
                    scripts->call(controls.stow, nullptr, nullptr, nullptr, 1, device);
                    replay = {};
                    snapshot.cancel();
                    ++lensEpoch;
                    lensRef = 0;
                    positioned = false;
                }
                break;
            default: break;
        }
    }
    if (wrist.open) refreshWrist();
}

void __cdecl wristChoose(Form*, void* parameters) {
    if (!loaded || !wrist.available || !wristBound) return;
    const auto x = argument(parameters, "button");
    if (x.type != 1 || !std::isfinite(x.value.number) || x.value.number < eyebot::WristPage::Tab ||
        x.value.number > (settingsOnly?eyebot::WristPage::EquipmentSubtab:eyebot::WristPage::SettingsLast) || std::floor(x.value.number) != x.value.number) return;
    dispatchWrist(static_cast<eyebot::WristPage::Button>(int(x.value.number)));
}

void __cdecl chooseCommand(Form*, void* parameters) {
    if (!loaded || !controlsBound) return;
    const auto key = argument(parameters, "choice");
    if (key.type != 1 || !std::isfinite(key.value.number) || key.value.number < 0 || key.value.number >= 5) return;
    auto device = form(call(controls.device));
    if (!device || device->id != lensRef) return;
    using Action = eyebot::UserControls::Action;
    switch (controls.choose(int(key.value.number))) {
        case Action::Stay:
            if (placeLens(device)) replay.park();
            break;
        case Action::Snap: takeSnapshot(device); break;
        case Action::Record: startRecording(device); break;
        case Action::Recall:
            if (replay.recall()) wantCommands(true);
            break;
        case Action::Save: saveRecording(device); break;
        case Action::Delete: controls.show(*gameConsole, 4); break;
        case Action::ConfirmDelete: replay.discard(); break;
        case Action::Stow:
            if (replay.state() == eyebot::State::Placed || replay.state() == eyebot::State::Parked) {
                scripts->call(controls.stow, nullptr, nullptr, nullptr, 1, device);
                reset();
            }
            break;
        case Action::Position:
            scripts->call(controls.moveBeside, nullptr, nullptr, nullptr, 1, device);
            placeLens(device);
            break;
        case Action::Preview:
            previewRequested = replay.state() == eyebot::State::Review;
            break;
        case Action::NewTake:
            if (movieSaved && replay.discard()) {
                movieSaved = false;
                placeLens(device);
                wantCommands(false);
            }
            break;
        case Action::Trash: if (movieSaved) controls.show(*gameConsole, 6); break;
        case Action::ConfirmTrash:
            if (trashMovie()) replay.discard();
            break;
        default: break;
    }
}

bool append(Form* reference, unsigned& count, bool isPlayer = false) {
    if (!reference || reference->id == lensRef) return true;
    auto root = scene::root(reference);
    if (!root || !scene::parent(root) || (!isPlayer && (scene::field<unsigned>(root, 0x30) & 1u))) return true;
    const auto transform = scene::world(root);
    const auto centre = eyebot::native_scene::metres(transform.p);
    constexpr float ConservativeRadius = 1.35f;
    if (!replay.lens().sees(centre, ConservativeRadius)) return true;
    // TESObjectREFR::baseForm, xNVSE GameObjects.h, runtime 1.4.0.525.
    auto base = scene::field<Form*>(reference, 0x20);
    const auto pose = eyebot::native_scene::pose(transform);
    if (!base || !pose.valid()) return false;
    // A missing pose or overflowing scene stops this take. Never silently
    // drop a visible object from an otherwise successful frame.
    if (count == sampleActors.size()) return false;
    auto& entity = sampleActors[count];
    entity = {reference->id, base->id, pose, centre, ConservativeRadius};
    if (isPlayer || reference->type == 0x3B || reference->type == 0x3C) {
        auto& rig = sampleRigs[count];
        rig.count = rig.traversed = rig.brokenPanes = rig.paneCount = 0;
        rig.layout = 2166136261u;
        if (!eyebot::native_scene::collectRig(root, rig)) return false;
        entity.flags = rig.brokenPanes;
        if (rig.count) {
            entity.joints = rig.joints;
            entity.jointCount = rig.count;
            entity.asset |= uint64_t(rig.layout) << 32;
            if (isPlayer && playerRigs.size() < 32) {
                const auto [entry, inserted] = playerRigs.try_emplace(rig.layout);
                if (inserted) {
                    entry->second.reserve(rig.count);
                    for (uint32_t i = 0; i < rig.count; ++i) {
                        entry->second.emplace_back(scene::name(rig.nodes[i]));
                    }
                }
            }
        }
    }
    ++count;
    return true;
}

void sample(uint64_t now) {
    auto cell = form(call(getCell));
    if (!cell) {
        replay.recall();
        return;
    }
    const auto source = call(getReferences);
    if (source.type != 4 || !source.value.array) {
        replay.recall();
        return;
    }
    const auto size = arrays->size(source.value.array);
    if (size > ReferenceLimit || (size && !arrays->elements(source.value.array, references.data(), nullptr))) {
        replay.recall();
        return;
    }
    unsigned count = 0;
    const auto player = form(call(getPlayer));
    if (!append(player, count, true)) {
        replay.recall();
        return;
    }
    for (unsigned index = 0; index < size; ++index) {
        auto reference = form(references[index]);
        if (reference != player && !append(reference, count)) {
            replay.recall();
            return;
        }
    }
    replay.sample(now, cell->id, sampleActors.data(), count);
}

void __cdecl message(Message* incoming) {
    switch (incoming->type) {
        case DeferredInit:
            wristReady = wrist.compile(*scripts, settingsOnly, recorderInstalled);
            getPlayer = scripts->expression("player");
            if (settingsOnly && wristReady) {
                eyebot::full_page::stockTab=[](int index){eyebot::native_tabs::request=eyebot::native_tabs::exitFullBase+index;};
                eyebot::full_page::click=[](int id){dispatchWrist(static_cast<eyebot::WristPage::Button>(id));};
                eyebot::native_tabs::install(paintNativeWrist);
                if(eyebot::native_tabs::installed)wristHardware.initialize(*scripts);
            }
            voiceReady=wristReady&&robco_voice::Capture::available()&&voice.compile(*scripts,*arrays);
            if(!recorderInstalled)break;
            controlScripts = controls.compile(*scripts);
            // Exterior cell borders are streaming boundaries; a parked lens and
            // passing cars still share one world. Interiors keep cell identity.
            getCell = scripts->compile(R"(
ref sceneId
begin function {}
    let sceneId := player.GetParentWorldspace
    if eval !sceneId
        let sceneId := player.GetParentCell
    endif
    SetFunctionValue sceneId
end
)");
            menuMode = scripts->expression("MenuMode");
            findDevices = scripts->expression(R"(GetFormRefs (GetFormFromMod "RobCo Eyebot.esp" "000800"))");
            recorderCamera.compile(*scripts);
            getReferences = scripts->compile(R"(
array_var actors
array_var items
begin function {}
    let actors := GetActorsByProcessingLevel 0
    let items := GetRefs 201 0 1
    ar_InsertRange actors (ar_Size actors) items
    SetFunctionValue actors
end
)");
            break;
        case PostLoadGame:
        case NewGame:
            reset();
            controlsBound = false;
            authoredDeviceId = 0;
            loaded = (incoming->type == NewGame || incoming->data != nullptr) &&
                     getPlayer && (settingsOnly ? wristReady.load() : getCell && getReferences && menuMode);
            break;
        case ExitGame:
            voice.shutdown();
            [[fallthrough]];
        case PreLoadGame:
        case ExitMenu:
            loaded = false;
            reset();
            break;
        case MainLoop:
            if (!loaded) break;
            tickWrist();
            if (!recorderInstalled) break;
            if (previewRequested && replay.state() == eyebot::State::Review && !replay.frames().empty()) {
                const auto menu = call(menuMode);
                if (menu.type == 1 && menu.value.number == 0) {
                    previewRequested = false;
                    renderTake(replay.frames()[replay.frames().size()/2].timeUs);
                }
            }
            if ((controlScripts && !controlsBound) || showCommands) {
                const auto menu = call(menuMode);
                if (menu.type == 1 && menu.value.number == 0) {
                    if (!controlsBound) {
                        auto stock = form(call(controls.device));
                        if (stock) authoredDeviceId = stock->id;
                        controlsBound = stock && controls.registerForGame(*scripts, form(call(controls.remote)));
                        if (!controlsBound) controlScripts = false;
                    }
                    if (showCommands) populateCommands();
                }
            }
            processMovie();
            if (snapshot.waiting()) {
                const auto menu = call(menuMode);
                renderSnapshot(wrist.open || (menu.type == 1 && menu.value.number == 0));
            }
            if (replay.state() != eyebot::State::Recording) break;
            const auto now = microseconds();
            if (now < nextSample) break;
            nextSample = now + eyebot::Replay::PeriodUs;
            const auto menu = call(menuMode);
            if (menu.type == 1 && menu.value.number == 0) {
                sample(now);
                const auto elapsed = microseconds() - now;
                samplingTime += elapsed;
                maxSampleTime = std::max(maxSampleTime, elapsed);
                ++sampleCalls;
            }
            break;
    }
}

uint32_t pipboyTrigger(int kind,float pressure) {
    static bool down=false,owned=false;
    if(!(kind&8)||!std::isfinite(pressure)||pressure<=.20f){down=owned=false;return 0;}
    const auto pointer=filthyPointer.load();
    const auto focus=pointer>>5;
    const auto hit=unsigned(pointer&31);
    const auto now=GetTickCount64();
    const bool categoryGrip=fullPipboyGrips.load()!=0;
    unsigned action=0;
    if(!categoryGrip&&fullPipboyReady&&focus&&now>=focus&&now-focus<220){
        if(wristInputOpen&&hit>=pages::Row0&&hit<=pages::MoreButton&&(visibleButtons.load()&(1u<<hit))){
            constexpr unsigned buttons[]{7120,7121,7122,7123,7124,7116,7117,7118};
            if(!down&&pressure>.55f)pointedButton=buttons[hit-pages::Row0];
            if(!down&&pressure>.55f){down=owned=true;}
            return 1;
        }
        if(hit==pages::CategoryTab&&!wristInputOpen)action=SelectFilthyTab;
        else if(hit>=pages::GeneralTab&&hit<=pages::EquipmentTab&&wristInputOpen){
            const auto page=static_cast<pages::FilthyPage>(pages::General+hit-pages::GeneralTab);
            constexpr unsigned selections[]{SelectGeneral,SelectEyebot,SelectVoice,SelectPool,SelectWorkshop,SelectEquipment};
            const auto state=fullFilthyPageState.load();
            if((state&pages::bit(page))&&page!=(state>>8))action=selections[hit-pages::GeneralTab];
        }
    }
    if(!down&&pressure>.55f){
        down=true;
        owned=action!=0;
        if(owned)wristInputEdges.fetch_or(action);
    }
    if(categoryGrip)owned=false;
    return owned||(!down&&action)?1:0;
}

uint32_t __cdecl wristInput(const char* name, int hand, int kind, float x, float y) {
    if(hand==1&&wristWorkshopActive.load()&&!wristInputOpen.load()&&!fullPipboyReady&&
       (!std::strcmp(name,"primary_click")||!std::strcmp(name,"secondary_click")||!std::strcmp(name,"thumbstick")))return 0;
    if(settingsOnly&&hand>=0&&hand<2&&(kind&3)==1&&!std::strcmp(name,"squeeze")){
        const auto mask=1u<<hand;
        if((kind&8)&&x>.55f){
            const auto was=fullPipboyGrips.fetch_or(mask);
            if(!(was&mask)&&eyebot::native_tabs::ownsFullInput()&&wristInputOpen&&!categoryTriggerHeld)
                fullPageGripArmed.fetch_or(mask);
        }else if(!(kind&8)||x<.30f){
            fullPipboyGrips.fetch_and(~mask);
            if((fullPageGripArmed.fetch_and(~mask)&mask)&&eyebot::native_tabs::ownsFullInput()&&wristInputOpen&&!categoryTriggerHeld)
                wristInputEdges.fetch_or(hand?NextFullPage:PreviousFullPage);
        }
    }
    if(hand>=0 && hand<2 && (kind&3)==1 && !std::strcmp(name,"trigger")){
        if(hand==1){
            categoryTriggerHeld=(kind&8)&&x>.20f;
            if(categoryTriggerHeld)fullPageGripArmed=0;
        }
        if (liftControl.update(hand, (kind & 8) ? x : 0.f,
                               pilotActive.load() && wristInputOpen.load(), GetTickCount64())) return 1;
    }
    if(settingsOnly&&hand==1&&(kind&3)==1&&!std::strcmp(name,"trigger"))return pipboyTrigger(kind,x);
    if ((kind & 3) == 0) {
        unsigned bit = 0;
        if (hand == 1 && !std::strcmp(name, "thumbstick_click")) bit = 1;
        else if (hand == 1 && !std::strcmp(name, "primary_click")) bit = 2;
        else if (hand == 1 && !std::strcmp(name, "secondary_click")) bit = 4;
        else if (hand == 0 && !std::strcmp(name, "primary_click")) bit = 8;
        if (!bit) return 0;
        static unsigned held = 0, consumed = 0;
        const bool down = (kind & 8) && x > .55f;
        const bool open=wristInputOpen.load();
        bool owns = bit == 1 && !eyebot::native_tabs::installed ? wristInputAvailable.load() : open;
        // At the worn General root, Back belongs to the same native input path
        // as the other categories. Only an actual submenu consumes this press.
        if (settingsOnly && bit == 4 && owns && down && !(held & bit) && liveWristOpen) {
            std::lock_guard<std::mutex> lock(wristImageMutex);
            if (wristImage.settings && wristImage.activeFilthyPage == pages::General &&
                wristImage.settingsView.page == fc_settings::WristHome) owns = false;
        }
        if (down && !(held & bit)) {
            held |= bit;
            if(!qaDirectory.empty()&&eyebot::native_tabs::selection!=eyebot::native_tabs::Selection::Stock){
                std::ofstream trace(qaDirectory+"\\filthy-input.csv",std::ios::app);
                trace<<GetCurrentProcessId()<<','<<GetTickCount64()<<','<<name<<','<<hand<<','<<owns<<','<<open<<'\n';
            }
            if (owns) {
                consumed |= bit;
                wristInputEdges.fetch_or(bit);
            }
        } else if (!down) {
            held &= ~bit;
            consumed &= ~bit;
        }
        return (consumed & bit) ? 1 : 0;
    }
    if ((kind & 3) != 2 || hand < 0 || hand > 1 || std::strcmp(name,"thumbstick"))return 0;
    if(hand==1){
        const bool down=(kind&8)&&std::isfinite(x)&&std::isfinite(y)&&x*x+y*y>.04f;
        if(wristInputOpen&&(wristSettingsInput||(wristEyebotInput&&!pilotActive))){
            settingsX=down?x:0.f;settingsY=down?y:0.f;settingsAt=GetTickCount64();if(down)settingsStickDrain=true;return 1;
        }
        if(settingsStickDrain){if(!wristInputOpen || !down)settingsStickDrain=false;else return 1;}
    }
    if(!pilotActive.load())return 0;
    const bool active = (kind & 8) && std::isfinite(x) && std::isfinite(y);
    stick[hand][0] = active ? std::clamp(x, -1.f, 1.f) : 0.f;
    stick[hand][1] = active ? std::clamp(y, -1.f, 1.f) : 0.f;
    stickTime[hand] = GetTickCount64();
    return 1;
}

}

extern "C" __declspec(dllexport) const fc_interaction::Provider* __cdecl FC_InteractionProviderV1() {
    static const fc_interaction::Provider provider{sizeof(provider), fc_interaction::version, wristInput, nullptr};
    return &provider;
}

extern "C" __declspec(dllexport) bool __cdecl FC_WristOwnsFieldInput() {
    return wristInputOpen.load();
}

extern "C" __declspec(dllexport) bool __cdecl FC_WristWorkshopHostReady() {
    return settingsOnly && wristReady.load() && eyebot::native_tabs::installed;
}

extern "C" __declspec(dllexport) unsigned __cdecl FC_WristSettingsHostVersion() { return wristReady.load() ? 1u : 0u; }

extern "C" __declspec(dllexport) unsigned __cdecl FC_FilthyPointerLayout(){
    return (fullFilthyPageState.load()&0xffu)|(compactSettings.load()?0x100u:0);
}
extern "C" __declspec(dllexport) void __cdecl FC_FilthyPointed(unsigned hit) {
    filthyPointer=hit&&hit<=pages::MoreButton&&fullPipboyReady.load()?(GetTickCount64()<<5)|hit:0;
}

extern "C" __declspec(dllexport) unsigned __cdecl FC_EyebotNativeCrt(unsigned flags) {
    using namespace eyebot::wrist_frame;
    if (flags & Visible) liveWristAt = GetTickCount64();
    if (flags & Focused) focusedWristAt = GetTickCount64();
    if(settingsOnly && eyebot::native_tabs::installed &&
       eyebot::native_tabs::wornCategory()==eyebot::native_tabs::filthyWornCategory)return 0;
    return (liveWristOpen.load()||wristWorkshopActive.load()) && nativeWristReady.load() ? 1u : 0u;
}

extern "C" __declspec(dllexport) bool __cdecl NVSEPlugin_Query(const NVSE* nvse, PluginInfo* info) {
    *info = {1, settingsOnly ? "FilthyCasualNativeSettings" : "RobCoEyebot", 1};
    return !nvse->isEditor && nvse->runtime == 0x040020D0;
}

extern "C" __declspec(dllexport) bool __cdecl NVSEPlugin_Load(const NVSE* nvse) {
    scripts = static_cast<Scripts*>(nvse->queryInterface(6));
    arrays = static_cast<Arrays*>(nvse->queryInterface(5));
    gameConsole = static_cast<Console*>(nvse->queryInterface(1));
    auto events = static_cast<Events*>(nvse->queryInterface(8));
    auto messages = static_cast<Messaging*>(nvse->queryInterface(2));
    if (!scripts || !arrays || !gameConsole || !events || !messages) return false;
    LARGE_INTEGER frequency{};
    if (!QueryPerformanceFrequency(&frequency) || frequency.QuadPart <= 0) return false;
    clockHz = static_cast<uint64_t>(frequency.QuadPart);
    char directory[MAX_PATH]{};
    const auto length = GetEnvironmentVariableA("SEATED_TEST_DIR", directory, MAX_PATH);
    if (length && length < MAX_PATH) qaDirectory.assign(directory, length);
    if (settingsOnly) {
        const auto present=[](const wchar_t* path){
            const auto attrs=GetFileAttributesW(path);
            return attrs!=INVALID_FILE_ATTRIBUTES&&!(attrs&FILE_ATTRIBUTE_DIRECTORY);
        };
        recorderInstalled=present(L"Data\\RobCo Eyebot.esp")&&
            present(L"Data\\Meshes\\RobCoEyebot\\Recorder.nif")&&
            present(L"Data\\Textures\\RobCoEyebot\\screen.dds")&&
            !GetModuleHandleW(L"RobCoEyebot.dll");
    }
    if (!recorderInstalled) {
        return events->registerEvent("RobCo:EyebotWrist", 0, nullptr, 6) &&
               events->listen("RobCo:EyebotWrist", wristChoose) &&
               messages->listen(nvse->getPluginHandle(), "NVSE", message);
    }
    const struct { const char* name; void (__cdecl* handler)(Form*, void*); } commands[] = {
        {"RobCo:EyebotPlace", place}, {"RobCo:EyebotStay", stay},
        {"RobCo:EyebotRecord", start}, {"RobCo:EyebotRecall", recall},
        {"RobCo:EyebotDiscard", discard},
        {"RobCo:EyebotSnap", requestSnapshot},
        {"RobCo:EyebotSave", save},
        {"RobCo:EyebotRemote", remoteCommands},
        {"RobCo:EyebotChoice", chooseCommand},
        {"RobCo:EyebotWrist", wristChoose},
    };
    for (auto& command : commands) {
        if (!events->registerEvent(command.name, 0, nullptr, 6) ||
            !events->listen(command.name, command.handler)) return false;
    }
    return events->listen("OnActivate", nativeActivate) &&
           messages->listen(nvse->getPluginHandle(), "NVSE", message);
}
