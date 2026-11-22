#pragma once
#include "../settings_ui.hpp"
#include <deque>
#include <map>
#include <string>
#include <vector>

namespace fort_messages {
enum Request { Count, Read, Choose, Present };
using Access = unsigned (__cdecl*)(int,unsigned,int,int,int,fc_settings::View*,unsigned);
struct Button { int original=0; std::string label; };
struct Message {
    unsigned script=0, object=0, token=0; bool interactive=false;
    std::string body; std::vector<Button> buttons;
};
struct Mailbox {
    std::deque<Message> messages;
    std::map<std::pair<unsigned,unsigned>,int> replies;
    unsigned sequence=0;
    bool push(Message m) {
        // JIP preserves up to 8191 bytes per co-save string. Longer authored
        // prompts must stay with their original game presenter.
        if (messages.size()>=64 || m.body.size()>8191 || m.buttons.size()>16) return false;
        for(const auto& button:m.buttons)if(button.label.size()>8191)return false;
        if (!m.interactive && !messages.empty() && messages.back().body==m.body) return true;
        if (++sequence==0) ++sequence;
        m.token=sequence;
        if (m.interactive) replies.erase({m.script,m.object});
        messages.push_back(std::move(m)); return true;
    }
    bool choose(unsigned token,int button) {
        if(messages.empty()||messages.front().token!=token)return false;
        const auto& m=messages.front();
        if(button<0||button>=int(m.buttons.size()))return false;
        if(m.interactive)replies[{m.script,m.object}]=m.buttons[button].original;
        messages.pop_front();return true;
    }
    bool poll(unsigned script,unsigned object,double& result) {
        const auto key=std::make_pair(script,object);
        auto it=replies.find(key);
        if(it!=replies.end()){result=it->second;replies.erase(it);return true;}
        for(const auto& m:messages)if(m.interactive&&m.script==script&&m.object==object){result=-1;return true;}
        return false;
    }
};
inline Mailbox inbox;

inline std::vector<std::string> textPages(const std::string& source) {
    std::string text;
    for(unsigned char c:source) {
        if(c=='\r'||c=='\n'||c=='\t'||c==' '){if(!text.empty()&&text.back()!=' ')text+=' ';}
        else text+=char(c);
    }
    std::vector<std::string> pages;
    for(size_t start=0;start<text.size();){
        auto end=std::min(start+size_t(128),text.size());
        if(end<text.size()) {
            auto space=text.rfind(' ',end);
            if(space!=text.npos&&space>start)end=space;
        }
        pages.push_back(text.substr(start,end-start));start=end;
        while(start<text.size()&&text[start]==' ')++start;
    }
    if(pages.empty())pages.emplace_back();
    return pages;
}
inline bool render(const Message& m,int textPage,int choicePage,fc_settings::View& view) {
    view={};view.page=fc_settings::WristSettlement;view.epoch=m.token;
    strcpy_s(view.title,"MOD MESSAGES");
    const auto text=textPages(m.body);
    textPage=std::max(0,textPage)%int(text.size());
    const int count=std::max(1,int(m.buttons.size()+5)/6);
    choicePage=std::max(0,choicePage)%count;
    strncpy_s(view.note,text[textPage].c_str(),_TRUNCATE);
    auto row=[&](const std::string& label,const std::string& value,int kind){
        auto& r=view.rows[view.count++];strncpy_s(r.label,label.c_str(),_TRUNCATE);
        strncpy_s(r.detail,label.c_str(),_TRUNCATE);strncpy_s(r.value,value.c_str(),_TRUNCATE);r.kind=kind;
    };
    for(int i=choicePage*6;i<std::min(int(m.buttons.size()),choicePage*6+6);++i)row(m.buttons[i].label,"",fc_settings::Action);
    if(text.size()>1)row("Message text",std::to_string(textPage+1)+" / "+std::to_string(text.size()),fc_settings::Number);
    if(count>1)row("More choices",std::to_string(choicePage+1)+" / "+std::to_string(count),fc_settings::Number);
    return true;
}
}
