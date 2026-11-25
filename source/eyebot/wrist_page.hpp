#pragma once
#include "../nvse_function.hpp"
#include "../settings_ui.hpp"
#include "wrist_frame.hpp"
#include "full_filthy_menu.hpp"
#include <iterator>
#include <vector>

namespace eyebot {

class WristPage {
public:
    enum Button : int {
        Tab = 7101, Deploy, Stay, Snap, Record, Save, Clips, Navigate, Stow, Back,
        Previous, Next, Trash, ConfirmTrash,
        SettingsTab=7115, SettingsBack, SettingsLess, SettingsMore, SettingsCycle,
        SettingsRow0=7120, SettingsLast=7124, GeneralSubtab, EyebotSubtab, VoiceSubtab, PoolSubtab, WorkshopSubtab, EquipmentSubtab
    };

    bool compile(Scripts& source, bool settingsOnly = false,bool recorder = true) {
        settingsOnly_ = settingsOnly;
        recorder_ = recorder;
        scriptApi = &source;
        menuMode = source.expression("GetActiveMenuMode");
        visibility = fc::nvse::loadFunction(source, "Wrist/ReadTile");
        for (auto& menu : menus) {
            const auto root = std::string(menu.name) + "/GLOW_BRANCH";
            menu.stock = source.expression(("IsComponentLoaded \"" + root + "/NVVR_PipboyTabs\"").c_str());
            menu.installed = source.expression(("IsComponentLoaded \"" + root + "/RobCoEyebotWrist\"").c_str());
            menu.add = source.expression(("InjectUIXML \"" + root + "\" \"Data\\Menus\\prefabs\\RobCoEyebot\\wrist.xml\"").c_str());
            const auto row=root+"/NVVR_PipboyTabs";
            menu.hasButton=source.expression(("IsComponentLoaded \""+row+"/FC_TabFilthy\"").c_str());
            menu.addButton=source.expression(("InjectUIXML \""+row+"\" \"Data\\Menus\\prefabs\\RobCoEyebot\\filthy_tab.xml\"").c_str());
            if (!menu.stock || !menu.installed || !menu.add || !menu.hasButton || !menu.addButton) return false;
        }
        input = fc::nvse::loadFunction(source, "Wrist/ReadSticks");
        click = fc::nvse::loadFunction(source, "Wrist/Click");
        bind = fc::nvse::loadFunction(source, "Wrist/BindClick");
        if (!recorder_) return menuMode && visibility && input && click && bind;
        shift = fc::nvse::loadFunction(source, "Eyebot/Move");
        moveClear = fc::nvse::loadFunction(source, "Eyebot/CheckPath");
        return menuMode && visibility && input && click && bind && shift && moveClear;
    }

    bool settingsOnly_ = false,recorder_ = true;

    struct Menu { int id; const char* name; Script *stock{}, *installed{}, *add{}, *hasButton{}, *addButton{}; };
    Menu custom{full_page::id,"FilthyMenu"};
    bool independent() const {return selected==&custom;}
    bool prepareDataPanel() {
        if (!settingsOnly_ || !selected || selected->id != full_page::dataCategory) return false;
        auto root = full_page::registered(full_page::dataCategory);
        constexpr auto panelPath = "GLOW_BRANCH/RobCoEyebotWrist";
        if (auto panel = full_page::find(root, panelPath)) return full_page::find(panel, "Settings") != nullptr;
        Element result;
        if (!scriptApi || !scriptApi->call(selected->add, nullptr, nullptr, &result, 0)) return false;
        auto panel = full_page::find(root, panelPath);
        full_page::number(panel, "visible", 0.f);
        return full_page::find(panel, "Settings") != nullptr;
    }
    bool openDataPanel() {
        if (!prepareDataPanel()) return false;
        dataPanelHost = full_page::registered(full_page::dataCategory);
        return dataPanelHost != nullptr;
    }
    bool dataPanelLost() const {
        return dataPanelHost && full_page::registered(full_page::dataCategory) != dataPanelHost;
    }
    Menu* stockMenu(int id) {
        if(settingsOnly_ && id==custom.id && full_page::instance)return &custom;
        for (auto& menu : menus) if (menu.id == id) return &menu;
        return nullptr;
    }
    Menu* selected{};

    bool bindButtons(Scripts& source) const {
        for (const auto& menu : menus) for (int button = Tab; button <= (settingsOnly_?Tab:SettingsLast); ++button) {
            Element result;
            if (!recorder_ && button != Tab && (button < SettingsBack && button != SettingsTab)) continue;
            const auto path = std::string(menu.name) + "#" + std::to_string(button);
            if (!source.call(bind, nullptr, nullptr, &result, 2, click, path.c_str()) ||
                result.type != 1 || result.value.number != 1) return false;
        }
        return true;
    }

    void close(Console& console) {
        if (dataPanelLost()) stockVisibility.clear();
        else if (open) { display(console, false); set(console,"Settings/visible",false); }
        dataPanelHost = nullptr;
        open = library = confirming = false;
    }

    void display(Console& console, bool visible) {
        if (visible != open && !independent()) showStock(console,visible);
        set(console, "Page/visible", visible);
        open = visible;
        if(auto root=panelRoot()){
            if(dataPanelHost)full_page::value(root,"GLOW_BRANCH/RobCoEyebotWrist/visible",visible);
            full_page::value(root,"GLOW_BRANCH/NVVR_PipboyTabs/FC_TabFilthy/_selected",open);
        }
        else if(selected)setVisibility(console,std::string(selected->name)+"/GLOW_BRANCH/NVVR_PipboyTabs/FC_TabFilthy/_selected",open);
        if(settingsOnly_){
            if(!open)set(console,"FilthyPages/visible",false);
        }
    }

    void maintainStockVisibility(Console& console) const {
        if (independent() || !settingsOnly_ || !open || !scriptApi || !visibility) return;
        for (const auto& tile : stockVisibility) {
            Element visible{};
            if (scriptApi->call(visibility,nullptr,nullptr,&visible,1,tile.path.c_str()) &&
                visible.type==1 && visible.value.number!=0)
                setVisibility(console,tile.path,false);
        }
    }

    void settingsDisplay(Console& console,bool visible) const {
        set(console,"Page/visible",open&&!visible);
        set(console,"Settings/visible",open&&visible);
        if(settingsOnly_){
            set(console,"Settings/TabsPrompt/visible",false);
            set(console,"Settings/Cycle/visible",false);
            set(console,"Settings/Cycle/target",false);
            set(console,"Page/SettingsTab/visible",false);
            set(console,"Page/SettingsTab/target",false);
        }
    }

    void filthyTabs(Console& console,wrist_frame::FilthyPage active,uint8_t availablePages) const {
        if(!settingsOnly_||!selected||!open)return;
        using namespace wrist_frame;
        constexpr FilthyPage pages[]{General,Eyebot,Voice,Pool,Workshop,Equipment};
        constexpr const char* paths[]{"FilthyPages/General","FilthyPages/Eyebot","FilthyPages/Voice","FilthyPages/Pool","FilthyPages/Workshop","FilthyPages/Equipment"};
        if (availablePages == bit(General)) return;
        set(console,"FilthyPages/visible",true);
        auto root=panelRoot();
        if(root){
            constexpr const char* icons[]{"","general","eyebot","voice","pool","build","equipment"};
            const auto icon=full_page::find(root,"GLOW_BRANCH/RobCoEyebotWrist/SectionIcon");
            const auto file=std::string("FilthyCasual/Device/")+icons[active]+".dds";
            full_page::text(icon,"filename",file.c_str());full_page::number(icon,"visible",1);
        }
        unsigned count=0,visibleIndex=0;
        for(auto page:pages)if(availablePages&bit(page))++count;
        for(unsigned i=0;i<std::size(pages);++i){
            const auto path=std::string(paths[i]);
            const bool present=(availablePages&bit(pages[i]))!=0;
            if(present&&root){
                auto node=full_page::find(root,"GLOW_BRANCH/RobCoEyebotWrist/"+path);
                full_page::number(node,"_x",960.f*(.5f+visibleIndex++)/count);
            }
            set(console,(path+"/visible").c_str(),present);
            set(console,(path+"/target").c_str(),present);
            set(console,(path+"/_selected").c_str(),active==pages[i]);
        }
    }

    void settingsContent(Console& console,const fc_settings::View& view,const std::string& picture={}) const {
        if(auto root=panelRoot()){
            const std::string base="GLOW_BRANCH/RobCoEyebotWrist/Settings/";
            auto image=full_page::find(root,base+"EquipmentLayout");
            if(!picture.empty())full_page::text(image,"filename",picture.c_str());
            full_page::number(image,"visible",!picture.empty());
            for(int i=0;i<fc_settings::DeviceRows;++i){
                const auto row=base+"Row"+std::to_string(i);
                full_page::number(full_page::find(root,row),"width",picture.empty()?840.f:530.f);
                full_page::number(full_page::find(root,row+"/Value"),"x",picture.empty()?550.f:295.f);
            }
        }
        label(console,"Settings/Title",settingsOnly_ && view.page==fc_settings::WristHome ? "FILTHY / GENERAL SETTINGS" : view.title);
        const int count=std::min<int>(view.count,8);
        const int first=fc_settings::firstVisible(view);
        for(int i=0;i<fc_settings::DeviceRows;++i){
            const int index=first+i;
            const auto tile="Settings/Row"+std::to_string(i);
            set(console,(tile+"/visible").c_str(),index<count);
            set(console,(tile+"/target").c_str(),index<count&&view.rows[index].kind!=fc_settings::Unavailable);
            if(index>=count)continue;
            const auto& row=view.rows[index];

            auto clipped=[&](const char* value,size_t limit){
                std::string s=value;
                if(!picture.empty()&&s.size()>limit)s=s.substr(0,limit-3)+"...";
                return s;
            };
            label(console,(tile+"/Caption").c_str(),(unsigned(index)==view.selected?"> ":"  ")+clipped(row.label,21));
            label(console,(tile+"/Value").c_str(),clipped(row.value,18));
        }
        const auto* activeRow=count?&view.rows[std::min<unsigned>(view.selected,count-1)]:nullptr;
        label(console,"Settings/Detail",*view.note?view.note:activeRow?activeRow->detail:"");
        label(console,"Settings/SelectLabel",activeRow&&activeRow->kind==fc_settings::Toggle?"Toggle":
            activeRow&&!_strnicmp(activeRow->label,"Save",4)?"Save":"Select");
        const bool selectable=count && activeRow->kind!=fc_settings::Unavailable;
        set(console,"Settings/SelectLabel/visible",selectable);set(console,"Settings/SelectPrompt/visible",selectable);
        const bool scroll = count>fc_settings::DeviceRows;
        label(console,"Settings/Less",scroll?"Page up":"Less");
        label(console,"Settings/More",scroll?"Page down":"More");
        set(console,"Settings/Less/visible",scroll?first>0:activeRow&&activeRow->kind==fc_settings::Number);
        set(console,"Settings/More/visible",scroll?first+fc_settings::DeviceRows<count:activeRow&&activeRow->kind==fc_settings::Number);
    }

    void showLibrary(Console& console, bool enable) {
        library = enable;
        confirming = false;
        set(console, "Page/Flight/visible", !enable);
        set(console, "Page/Library/visible", enable);
        set(console, "Page/Library/ConfirmTrash/visible", false);
    }

    void showConfirmation(Console& console, bool enable) {
        confirming = enable;
        set(console, "Page/Library/ConfirmTrash/visible", enable);
    }

    void highlight(Console& console, int button) const {
        constexpr const char* tiles[]{
            "Page/Flight/Deploy", "Page/Flight/Stay", "Page/Flight/Snap", "Page/Flight/Record",
            "Page/Flight/Save", "Page/Flight/Clips", "Page/Flight/Navigate", "Page/Flight/Stow",
            "Page/Flight/Back", "Page/Library/Previous", "Page/Library/Next", "Page/Library/Trash",
            "Page/Library/ConfirmTrash"
        };
        for (int id = Deploy; id <= ConfirmTrash; ++id)
            set(console, (std::string(tiles[id - Deploy]) + "/_selected").c_str(), id == button);
    }

    void label(Console& console, const char* tile, const std::string& label) const {
        if (!selected) return;
        // The console drops an empty string argument. A blank glyph clears
        // text left in a reused tile when the next row has no value.
        auto safe=label.empty()?std::string(" "):label;
        for(auto& c:safe)if(c=='"'||c=='\n'||c=='\r')c=' ';
        if(auto root=panelRoot()){full_page::text(full_page::find(root,std::string("GLOW_BRANCH/RobCoEyebotWrist/")+tile),"string",safe.c_str());return;}
        const auto command = std::string("SetUIString \"") + path() + tile + "/string\" \"" + safe + "\"";
        console.run(command.c_str(), nullptr);
    }

    void preview(Console& console, const std::string& texture) const {
        if (!selected) return;
        if(auto root=panelRoot()){full_page::text(full_page::find(root,"GLOW_BRANCH/RobCoEyebotWrist/Page/Still"),"filename",texture.c_str());return;}
        const auto command = std::string("SetUIString \"") + path() + "Page/Still/filename\" \"" + texture + "\"";
        console.run(command.c_str(), nullptr);
    }

    Script *menuMode{}, *visibility{}, *input{}, *click{}, *bind{}, *shift{}, *moveClear{};
    bool open = false, library = false, confirming = false, available = false;
    uint64_t nextTabCheck{}, lastTick{};

private:
    // Compare against the live registration before restoring any saved traits.
    void* dataPanelHost=nullptr;
    void* panelRoot() const {
        if (independent()) return full_page::root;
        return dataPanelLost() ? nullptr : dataPanelHost;
    }
    Scripts* scriptApi=nullptr;
    struct Tile { std::string path; bool visible; };
    std::vector<Tile> stockVisibility;
    void showStock(Console& console,bool hide){
        if(!hide){
            for(const auto& tile:stockVisibility)setVisibility(console,tile.path,tile.visible);
            stockVisibility.clear();
            return;
        }
        if(!selected||!scriptApi||!visibility)return;
        constexpr const char* inventory[]{"IM_MainRect","IM_Tabline"};
        constexpr const char* data[]{"MM_MainRect","MM_Tabline"};
        constexpr const char* stats[]{"stats_headline_h1","stats_headline_v1","stats_headline_h2","stats_title",
            "stats_status_container","stats_special_container","stats_skills_container","stats_perks_container",
            "stats_genrep_container","stats_display_rect","stats_tailline"};
        const char* const* tiles=selected->id==1002?inventory:selected->id==1003?stats:data;
        const auto count=selected->id==1003?std::size(stats):std::size(inventory);
        const auto branch=std::string(selected->name)+"/GLOW_BRANCH/";
        for(size_t i=0;i<count;++i)rememberTrait(console,branch+tiles[i]+"/visible");
        if(settingsOnly_){
            for(auto tab:{"NVVR_TabStats","NVVR_TabItems","NVVR_TabData"})
                rememberTrait(console,branch+"NVVR_PipboyTabs/"+tab+"/_selected");
        }
    }
    void rememberTrait(Console& console,const std::string& trait){
        Element previous{};
        if(scriptApi->call(visibility,nullptr,nullptr,&previous,1,trait.c_str())&&previous.type==1){
            stockVisibility.push_back({trait,previous.value.number!=0});
            if(previous.value.number!=0)setVisibility(console,trait,false);
        }
    }
    static void setVisibility(Console& console,const std::string& path,bool visible){
        const auto command=std::string("SetUIFloat \"")+path+"\" "+(visible?"1":"0");
        console.run(command.c_str(),nullptr);
    }
    std::array<Menu, 3> menus{{{1002, "InventoryMenu"}, {1003, "StatsMenu"}, {1023, "MapMenu"}}};

    std::string path() const { return std::string(selected->name) + "/GLOW_BRANCH/RobCoEyebotWrist/"; }

    void set(Console& console, const char* trait, bool value) const {
        if (!selected) return;
        if(auto root=panelRoot()){full_page::value(root,std::string("GLOW_BRANCH/RobCoEyebotWrist/")+trait,value);return;}
        const auto command = std::string("SetUIFloat \"") + path() + trait + "\" " + (value ? "1" : "0");
        console.run(command.c_str(), nullptr);
    }
};

} // namespace eyebot
