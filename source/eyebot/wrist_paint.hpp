#pragma once

#include <windows.h>
#include <algorithm>
#include <cstring>
#include <iterator>
#include "wrist_frame.hpp"
#include "native_wrist_theme.hpp"
#include "device_art.hpp"

namespace eyebot::wrist_paint {
using wrist_frame::View;
constexpr int Width = 512, Height = 394;
inline void screenText(HDC dc, const char* value, int x, int y, int size, COLORREF tint) {
    if(native_theme::installed().text(dc,value,{x,y,Width,y+size},size,tint))return;
    const auto font = CreateFontA(-size, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
        FIXED_PITCH | FF_MODERN, "Consolas");
    const auto previous = SelectObject(dc, font);
    SetTextColor(dc, tint);
    SetBkMode(dc, TRANSPARENT);
    TextOutA(dc, x, y, value, int(std::strlen(value)));
    SelectObject(dc, previous);
    DeleteObject(font);
}

inline void fitText(HDC dc,const char* value,int x,int y,int right,int bottom,int size,COLORREF tint) {
    if(native_theme::installed().text(dc,value,{x,y,right,bottom},size,tint))return;
    const auto font=CreateFontA(-size,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,FIXED_PITCH|FF_MODERN,"Consolas");
    const auto previous=SelectObject(dc,font);
    SetTextColor(dc,tint);SetBkMode(dc,TRANSPARENT);
    RECT area{x,y,right,bottom};
    DrawTextA(dc,value,-1,&area,DT_SINGLELINE|DT_END_ELLIPSIS|DT_NOPREFIX);
    SelectObject(dc,previous);DeleteObject(font);
}

inline void prompt(HDC dc,unsigned button,const char* label,int x,int y,COLORREF tint) {
    if(!native_theme::installed().icon(dc,button,x,y+1,19,tint)){
        constexpr const char* fallback[]{"A","B","X","R3"};
        screenText(dc,fallback[button],x,y,21,tint);
    }
    screenText(dc,label,x+25,y,21,tint);
}

inline void pageGrip(HDC dc,wrist_frame::FilthyPage next,int y,COLORREF tint){
    if(next==wrist_frame::NoPage)return;
    const char* label=next==wrist_frame::General?"R GRIP GENERAL":next==wrist_frame::Eyebot?"R GRIP EYEBOT":next==wrist_frame::Pool?"R GRIP POOL":next==wrist_frame::Workshop?"R GRIP BUILD":next==wrist_frame::Equipment?"R GRIP EQUIPMENT":"R GRIP VOICE";
    fitText(dc,label,294,y,477,y+29,21,tint);
}

inline void filthyTabs(HDC dc,const View& view,COLORREF text,COLORREF quiet){
    using namespace wrist_frame;
    constexpr FilthyPage pages[]{General,Eyebot,Voice,Pool,Workshop,Equipment};
    constexpr const char* labels[]{"GENERAL","EYEBOT","VOICE","POOL","BUILD","EQUIP"};
    if(!view.availableFilthyPages)return;
    unsigned count=0,slot=0;
    for(auto page:pages)if(view.availableFilthyPages&bit(page))++count;
    const int width=440/int(count);
    for(unsigned i=0;i<std::size(pages);++i){
        if(!(view.availableFilthyPages&bit(pages[i])))continue;
        const int left=35+int(slot++) * width;
        const bool selected=view.activeFilthyPage==pages[i];
        if(selected){
            const RECT line{left,363,left+width-8,365};
            const auto brush=CreateSolidBrush(text);
            FillRect(dc,&line,brush);
            DeleteObject(brush);
        }
        fitText(dc,labels[i],left+4,370,left+width-8,393,19,selected?text:quiet);
    }
}

inline void settingsPage(HDC dc,const fc_settings::View& settings,COLORREF text,COLORREF quiet,[[maybe_unused]] wrist_frame::FilthyPage nextPage,const unsigned char* picture=nullptr) {
    const auto dim=RGB(GetRValue(text)*2/5,GetGValue(text)*2/5,GetBValue(text)*2/5);
    const auto rule=[&](int x,int y,int right,COLORREF colour){
        const RECT line{x,y,right,y+1};
        const auto brush=CreateSolidBrush(colour);
        FillRect(dc,&line,brush);
        DeleteObject(brush);
    };
    constexpr int left=34,right=475;
    fitText(dc,settings.page==fc_settings::WristHome?"FILTHY":settings.title,left+36,21,393,61,30,text);
    rule(left,61,right,quiet);

    const int count=std::min<int>(settings.count,8);
    if(!count)return;
    const int selected=std::clamp(int(settings.selected),0,count-1);
    const auto& focused=settings.rows[selected];
    const int first=std::clamp(selected-1,0,std::max(0,count-3));
    char position[16]{};
    sprintf_s(position,"%d / %d",selected+1,count);
    fitText(dc,position,405,32,right,58,21,quiet);
    const int rowRight=picture?263:right;
    if(picture){
        BITMAPINFO info{};
        info.bmiHeader={sizeof(BITMAPINFOHEADER),256,-256,1,32,BI_RGB};
        SetStretchBltMode(dc,HALFTONE);
        StretchDIBits(dc,268,69,205,205,0,0,256,256,picture,&info,DIB_RGB_COLORS,SRCCOPY);
    }
    for(int i=first;i<std::min(first+3,count);++i){
        const auto& row=settings.rows[i];
        const int y=74+(i-first)*47;
        const bool active=i==selected,canOpen=row.kind!=fc_settings::Unavailable;
        if(active){
            const RECT area{left,y,rowRight,y+41};
            const auto brush=CreateSolidBrush(RGB(GetRValue(text)/10,GetGValue(text)/10,GetBValue(text)/10));
            FillRect(dc,&area,brush);
            DeleteObject(brush);
            screenText(dc,">",left+6,y+9,23,text);
        }
        fitText(dc,row.label,left+23,y+8,rowRight-3,y+41,picture?24:28,active?text:canOpen?quiet:dim);
        if(i<first+2)rule(left,y+43,rowRight,dim);
    }

    const char* detail=*settings.note?settings.note:focused.detail;
    const bool showValue=*focused.value&&std::strcmp(focused.value,"OPEN");
    if(showValue){
        fitText(dc,focused.value,left+3,221,rowRight,260,picture?26:32,text);
    }
    const int detailTop=picture?280:showValue?262:225;
    if(!native_theme::installed().text(dc,detail,{left+3,detailTop,right,327},21,quiet,true))
        fitText(dc,detail,left+3,detailTop,right,327,21,quiet);
    rule(left,332,right,dim);
    if(focused.kind==fc_settings::Number)screenText(dc,"STICK L/R",left,343,21,text);
    else if(focused.kind!=fc_settings::Unavailable)
        prompt(dc,0,focused.kind==fc_settings::Toggle?"TOGGLE":"SELECT",left,343,text);
    if(settings.page!=fc_settings::WristHome)prompt(dc,1,"BACK",194,343,quiet);
#if defined(FC_SETTINGS_ONLY)
    if(settings.page==fc_settings::WristControls)prompt(dc,3,"MODE",345,343,quiet);
    else pageGrip(dc,nextPage,343,quiet);
#else
    prompt(dc,settings.page==fc_settings::WristControls?3:2,
           settings.page==fc_settings::WristControls?"MODE":"TABS",345,343,quiet);
#endif
}

inline void page(HDC dc, const View& view, const unsigned char* photograph) {
    const auto text = native_theme::installed().colour();
    const auto quiet = RGB(GetRValue(text)*4/5, GetGValue(text)*4/5, GetBValue(text)*4/5);
    const auto ground = RGB(0, 0, 0);
    const auto backingBrush = CreateSolidBrush(ground);
    const RECT backing{0, 0, Width, Height};
    FillRect(dc, &backing, backingBrush);
    DeleteObject(backingBrush);

    if (view.settings) {
        settingsPage(dc,view.settingsView,text,quiet,view.nextFilthyPage,view.hasStill?photograph:nullptr);
        device_art::glyph(dc,view.activeFilthyPage,34,23,31,text);
        filthyTabs(dc,view,text,quiet);
        return;
    }

    if (view.holsters) {
        constexpr const char* profiles[]{"INFINITY", "DIAMOND", "ARC"};
        char profile[48], previews[36], markers[36], movement[40];
        sprintf_s(profile,"LAYOUT  %s%s",profiles[std::min(view.profile,2u)],
                  view.holsterState & (1u << std::min(view.profile,2u)) ? "" : "  NEW");
        sprintf_s(previews,"PREVIEWS  %s",view.holsterState & 0x10 ? "ON" : "OFF");
        sprintf_s(markers,"EDIT MARKERS  %s",view.holsterMarkers ? "ON" : "OFF");
        sprintf_s(movement,"GRIP MOVEMENT  %s",view.holsterState & 0x20 ? "LOCKED" : "ON");
        const char* rows[]{profile,"LOAD LAYOUT","SAVE CURRENT",previews,markers,movement};
        screenText(dc,"FILTHY CASUALS",36,17,33,text);
        screenText(dc,"EYEBOT / [HOLSTERS]",36,55,23,quiet);
        for (unsigned i=0;i<std::size(rows);++i) {
            const int y=91+34*i;
            screenText(dc,i==view.holsterRow?">":" ",34,y,25,text);
            screenText(dc,rows[i],63,y,27,i==view.holsterRow?text:quiet);
        }
        if (*view.status) screenText(dc,view.status,37,304,25,text);
        prompt(dc,0,"SELECT",38,338,quiet);
        prompt(dc,2,"TABS",337,338,quiet);
        return;
    }

    const bool stowed = !view.library && !view.piloting && !std::strcmp(view.status, "STOWED");
    screenText(dc, stowed ? "FILTHY CASUALS / EYEBOT" : view.piloting ? "EYEBOT CAMERA" : "EYEBOT",
               67, 14, stowed ? 29 : 35, text);
    device_art::glyph(dc,wrist_frame::Eyebot,28,16,32,text);
    if (!stowed && !view.piloting) screenText(dc, view.status, 235, 15, 28, text);
    auto rule = CreatePen(PS_SOLID, 2, quiet);
    const auto previous = SelectObject(dc, rule);
    MoveToEx(dc, 24, 56, nullptr); LineTo(dc, 488, 56);
    MoveToEx(dc, 30, 292, nullptr); LineTo(dc, 482, 292);

    if (view.hasStill && !stowed) {
        BITMAPINFO snap{};
        snap.bmiHeader = {sizeof(BITMAPINFOHEADER), 256, -256, 1, 32, BI_RGB};
        if (view.piloting) {
            // Center the crop on the lens axis and keep square source pixels.
            StretchDIBits(dc, 30, 68, 452, 215, 0, 67, 256, 122,
                          photograph, &snap, DIB_RGB_COLORS, SRCCOPY);
        } else {
            StretchDIBits(dc, 326, 83, 154, 154, 0, 0, 256, 256,
                          photograph, &snap, DIB_RGB_COLORS, SRCCOPY);
        }
    } else if (view.piloting) {
        screenText(dc, "WAITING FOR SIGNAL", 65, 174, 33, quiet);
    }

    if (stowed) {
        const auto brush = SelectObject(dc, GetStockObject(NULL_BRUSH));
        Rectangle(dc, 25, 110, 487, 175);
        SelectObject(dc, brush);
        screenText(dc, "RobCo Eyebot Remote", 38, 123, 35, text);
        prompt(dc,0,"DEPLOY",38,301,text);
        if(view.nextFilthyPage)pageGrip(dc,view.nextFilthyPage,301,quiet);
        else prompt(dc,2,"SETTINGS",272,301,quiet);
    } else if (view.piloting) {
        screenText(dc,"LT DOWN   RT UP",33,43,19,quiet);
        prompt(dc,0,"PARK",38,301,text);
        pageGrip(dc,view.nextFilthyPage,301,quiet);
    } else {
        constexpr const char* actions[]{"Deploy / Recall", "Stay", "Take a Snap", "Record",
            "Save MP4", "Clips", "Fly / Aim", "Stow", "Back"};
        constexpr const char* clips[]{"Previous", "Next", "Trash", "Confirm trash"};
        const auto list = view.library ? clips : actions;
        const int count = view.library ? (view.confirm ? 4 : 3) : 9;
        const int selected = std::clamp(view.selection - (view.library ? 7111 : 7102), 0, count - 1);
        const int first = std::clamp(selected - 1, 0, count - 3);
        for (int i = first; i < first + 3; ++i) {
            const int y = 80 + (i - first)*59;
            if (i == selected) {
                const auto brush = SelectObject(dc, GetStockObject(NULL_BRUSH));
                Rectangle(dc, 25, y - 2, 323, y + 50);
                SelectObject(dc, brush);
            }
            screenText(dc, list[i], 38, y + 3, 35, i == selected ? text : quiet);
        }
        if (view.library && view.count) screenText(dc, view.clip, 33, 259, 22, quiet);
        if(view.library)prompt(dc,1,"BACK",38,304,text);
        else {
            prompt(dc,0,"SELECT",38,304,text);
            if(view.nextFilthyPage)pageGrip(dc,view.nextFilthyPage,304,quiet);
            else prompt(dc,2,"TABS",272,304,quiet);
        }
    }
    SelectObject(dc, previous);
    DeleteObject(rule);
    filthyTabs(dc,view,text,quiet);
}

}
