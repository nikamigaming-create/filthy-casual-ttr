#pragma once
#include <cstdint>

namespace settlement_crew {
enum Action { Roster, Jobs, CallOver, SendToWork, Assign, Directory, InspectFaction, RecruitFaction, ReadPolicy, TogglePolicy, CompareFactions, MojaveDirectory, CapitalDirectory, ReadFreeBuild, ToggleFreeBuild, BeginGuardPost, ReleaseGuardPost };
constexpr int Rows = 5;
struct Range {unsigned first{}, count{};};
constexpr unsigned directoryWidth(unsigned count){return count<=Rows?1:(count+Rows-1)/Rows;}
struct Row { uint32_t id{}; char name[64]{}, role[40]{}; };
struct Page { unsigned count{}, total{}; Row rows[Rows]{}; char note[104]{}; };
using Access = bool (__cdecl*)(int, uint32_t, unsigned, Page*, unsigned);
}
