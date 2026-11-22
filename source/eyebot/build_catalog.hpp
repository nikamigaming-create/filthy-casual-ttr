#pragma once
#include "../nvse_function.hpp"
#include "../settlement-vr/crew.hpp"
#include "../settings_ui.hpp"
#include "../build_adapter.hpp"
#include <vector>
#include <string>

namespace eyebot::build_catalog {
// Only query the five displayed entries. Some installed lists contain thousands.
struct Browser {
    Script* catalog{};
    Scripts* api{};
    Form* list{};
    std::vector<settlement_crew::Range> parents;
    settlement_crew::Range range{};
    std::string category, notice;
    unsigned total{};
    bool prepare(Scripts& scripts) {
        api=&scripts;
        if(!catalog)catalog=fc::nvse::loadFunction(scripts,"Forts/BrowsePieces");
        return catalog!=nullptr;
    }
    Element query(int op,int index=0) const {
        Element result{};
        api->call(catalog,nullptr,nullptr,&result,2,op,index);
        return result;
    }
    bool succeeded(int op,int index=0) const {
        const auto result=query(op,index);
        return result.type==1 && result.value.number>0;
    }
    Form* record(int op,int index=0) const {
        const auto result=query(op,index);
        return result.type==2?result.value.form:nullptr;
    }
    void refresh() {
        parents.clear();notice.clear();
        list=record(4);
        auto count=query(5);
        range={0,count.type==1 && count.value.number>0?unsigned(count.value.number):0};
        total=range.count;
        constexpr const char* stock[]={"Camp","Farm","Defense","Various","Idle markers","Blueprints","Fort plans","Custom plans","Signs","Machines","Landmarks","Classic","Settlements","Addons"};
        auto name=query(6);
        category="Pieces";
        if(name.type==3 && name.value.string){
            category=name.value.string;
            build_adapter::freeGameString(name.value.string);
        }else if(name.type==1 && name.value.number>=0 && name.value.number<14){
            category=stock[int(name.value.number)];
        }
    }
    bool open(Scripts& scripts) {
        if(!prepare(scripts)||!succeeded(0))return false;
        refresh();return range.count>0;
    }
    void paint(fc_settings::View& view) const {
        strcpy_s(view.title,"BUILD CATALOG");
        strcpy_s(view.rows[0].label,"Previous category");
        strcpy_s(view.rows[1].label,"Next category");
        view.count=2;
        const auto width=settlement_crew::directoryWidth(range.count);
        for(unsigned at=0;at<range.count;at+=width) {
            auto& row=view.rows[view.count++];
            const auto first=range.first+at;
            const auto count=std::min(width,range.count-at);
            if(count>1) {
                sprintf_s(row.label,"Pieces %u to %u",first+1,first+count);
                strcpy_s(row.value,"OPEN");
            } else {
                auto name=query(1,first);
                if(name.type==3 && name.value.string){
                    strncpy_s(row.label,name.value.string,_TRUNCATE);
                    build_adapter::freeGameString(name.value.string);
                }
                if(!*row.label)sprintf_s(row.label,"Piece %u",first+1);
            }
        }
        if(notice.empty())_snprintf_s(view.note,_TRUNCATE,"%s. Pieces %u to %u of %u. Select to preview.",category.c_str(),range.count?range.first+1:0,range.first+range.count,total);
        else strncpy_s(view.note,notice.c_str(),_TRUNCATE);
    }
    void choose(unsigned row) {
        if(record(4)!=list){refresh();return;}
        if(row<2){if(succeeded(3,row?1:-1))refresh();return;}
        const unsigned width=settlement_crew::directoryWidth(range.count);
        const unsigned at=(row-2)*width;
        if(at>=range.count)return;
        const settlement_crew::Range selected{range.first+at,std::min(width,range.count-at)};
        if(selected.count>1){parents.push_back(range);range=selected;notice.clear();}
        else if(succeeded(2,selected.first))notice="Preview selected. Lower or close the Pip-Boy to aim.";
        else notice="Selection unavailable. Return to Build and check placement.";
    }
    bool back() {
        if(parents.empty())return false;
        range=parents.back();parents.pop_back();notice.clear();return true;
    }
};
}
