#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <cstring>
#include <cstdint>
#include "scene_adapter.hpp"

// Pure wrist geometry and touch policy. The caller owns native NIF attachment,
// lifecycle and semantic actions. Sample on the native game/scene thread; never
// mix this frame's render mount with a different frame's fingertip for hit tests.
namespace wrist_controls {
using scene::Vec;
using scene::Transform;
constexpr float AssetUnitsPerMetre = 69.99125f;
constexpr float Cm = AssetUnitsPerMetre / 100.f;
constexpr float PressTravel = .16f * Cm; // 1.6 mm, in original asset-local units
constexpr float PressThreshold = .12f * Cm; // actuates after 1.2 mm depression
constexpr float MaxPressDepth = .80f * Cm; // 8 mm tracking tolerance, not mechanical travel
constexpr float ReleaseClearance = .50f * Cm; // withdraw 5 mm before rearming
enum Action { Weapons=0, Consumables=1, Reload=2, Settings=3, None=-1 };
inline float dot(Vec a, Vec b) { return a.x*b.x+a.y*b.y+a.z*b.z; }
inline bool finite(Vec v) { return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z); }
inline bool normalized(Vec v, Vec& out) {
    const float squared=dot(v,v);
    if(!finite(v)||!std::isfinite(squared)||squared<1e-8f) return false;
    out=v*(1.f/std::sqrt(squared));return true;
}
inline Vec axis(const Transform& t,int column) { return {t.r[0][column],t.r[1][column],t.r[2][column]}; }
inline Vec buttonLocal(int index) {
    switch(index){
    case Weapons:return {-1.65f*Cm,1.f*Cm,.48f*Cm};
    case Consumables:return {1.65f*Cm,1.f*Cm,.48f*Cm};
    case Reload:return {-1.65f*Cm,-1.30f*Cm,.46f*Cm};
    case Settings:return {1.65f*Cm,-1.30f*Cm,.46f*Cm};
    default:return {};
    }
}
inline float buttonRadius(int index) { return (index<2?1.20f:.90f)*Cm; }
inline Vec localPoint(const Transform& mount,Vec world) {
    return scene::inverseRotate(mount,world-mount.p)*(1.f/mount.scale);
}
inline Vec worldPoint(const Transform& mount,Vec local) {
    return mount.p+scene::rotate(mount,local)*mount.scale;
}
inline Vec buttonWorld(const Transform& mount,int index) { return worldPoint(mount,buttonLocal(index)); }
// Anatomical frame: forearm position is the elbow end, hand position the wrist.
// Dorsal/back-of-wrist normal is the negative of scene::palm's right-hand normal.
// Projection removes wrist flex from the forearm-long axis without gaze input.
inline bool mountingFrame(Vec forearm,Vec hand,Vec indexBase,Vec pinkyBase,
                          Transform& out,float unitsPerMetre=AssetUnitsPerMetre) {
    out={};
    if(!std::isfinite(unitsPerMetre)||unitsPerMetre<20.f||unitsPerMetre>200.f||
       !finite(forearm)||!finite(hand)||!finite(indexBase)||!finite(pinkyBase)) return false;
    Vec up,palm,across;
    const float armLength=scene::distance(hand,forearm)/unitsPerMetre;
    if(armLength<.08f||armLength>.60f||!normalized(forearm-hand,up)) return false;
    if(!normalized(scene::cross(indexBase-hand,pinkyBase-hand),palm)) return false;
    // Fail closed with an edge-on/degenerate hand; never invent a camera-facing axis.
    auto projected=palm-up*dot(palm,up);
    if(dot(projected,projected)<.04f||!normalized(projected,palm)) return false;
    palm=palm*(-1.f); // dorsal cuff, away from the palm
    if(!normalized(scene::cross(up,palm),across)) return false;
    palm=scene::cross(across,up);
    out.r[0][0]=across.x;out.r[1][0]=across.y;out.r[2][0]=across.z;
    out.r[0][1]=up.x;out.r[1][1]=up.y;out.r[2][1]=up.z;
    out.r[0][2]=palm.x;out.r[1][2]=palm.y;out.r[2][2]=palm.z;
    out.p=hand+up*(.04f*unitsPerMetre)+palm*(.026f*unitsPerMetre);
    out.scale=unitsPerMetre/AssetUnitsPerMetre;
    return finite(out.p);
}
inline bool facingEye(const Transform& mount,Vec eye) {
    if(!finite(eye)||!finite(mount.p)||!std::isfinite(mount.scale)||mount.scale<=0) return false;
    const auto toEye=eye-mount.p;const float distance=std::sqrt(dot(toEye,toEye));
    const float metres=distance/(AssetUnitsPerMetre*mount.scale);
    if(!std::isfinite(metres)||metres<.12f||metres>.90f) return false;
    // Front side only, no grazing/backside activation. Rendering may remain on.
    return dot(axis(mount,2),toEye)>distance*.35f;
}
struct Frame {
    Transform world{};
    void* anchor=nullptr;
    void* rightHand=nullptr;
    void* leftIndex=nullptr;
    Vec fingertip{},eye{};
    bool valid=false,facing=false;
};
inline Frame sample(void* rig,float unitsPerMetre=AssetUnitsPerMetre) {
    Frame f;if(!rig)return f;
    f.anchor=scene::find(rig,"Bip01 R Forearm");
    f.rightHand=scene::find(rig,"Bip01 R Hand");
    auto index=scene::find(rig,"Bip01 R Finger1");
    auto pinky=scene::find(rig,"Bip01 R Finger4");
    f.leftIndex=scene::find(rig,"Bip01 L Finger12");
    auto head=scene::find(rig,"Bip01 Head");
    if(!f.anchor||!f.rightHand||!index||!pinky||!f.leftIndex||!head)return f;
    f.fingertip=scene::world(f.leftIndex).p;f.eye=scene::world(head).p;
    f.valid=finite(f.fingertip)&&finite(f.eye)&&mountingFrame(scene::world(f.anchor).p,
        scene::world(f.rightHand).p,scene::world(index).p,scene::world(pinky).p,f.world,unitsPerMetre);
    f.facing=f.valid&&facingEye(f.world,f.eye);return f;
}
inline Vec buttonWorld(const Frame& frame,int index) { return buttonWorld(frame.world,index); }
// buttonLocal Z is the actual unpressed flat top face, not the node pivot.
// NIF *_Press children start at local translation zero and move only -Z.
struct PressSample {
    bool radial=false,valid=false;
    float signedDepth=0,depression=0;
};
inline PressSample physicalPress(Vec local,int index) {
    PressSample p;
    if(index<0||index>3||!finite(local))return p;
    const auto delta=local-buttonLocal(index);
    const float radius=buttonRadius(index);
    p.radial=delta.x*delta.x+delta.y*delta.y<=radius*radius;
    p.signedDepth=-delta.z;
    p.valid=p.radial&&p.signedDepth>=0&&p.signedDepth<=MaxPressDepth;
    if(p.valid)p.depression=std::clamp(p.signedDepth/PressTravel,0.f,1.f);
    return p;
}
inline float visualPress(const Frame& frame,int index) {
    if(!frame.valid||!frame.facing||!facingEye(frame.world,frame.eye))return 0;
    return physicalPress(localPoint(frame.world,frame.fingertip),index).depression;
}
struct TouchState {
    bool armed=false;
    bool approached[4]{};
    bool havePrevious=false;
    Vec previousLocal{};
    int candidate=None;
    std::uint64_t candidateSince=0,cooldownUntil=0,lastTime=0;
    void disarm(){armed=false;havePrevious=false;candidate=None;candidateSince=0;for(bool& a:approached)a=false;}
    void reset(){disarm();cooldownUntil=0;lastTime=0;}
    int update(const Frame& frame,std::uint64_t now,bool usable=true) {
        if(now<lastTime||(lastTime&&now-lastTime>500)){reset();lastTime=now;return None;}lastTime=now;
        if(!usable||!frame.valid||!frame.facing||!facingEye(frame.world,frame.eye)) {
            disarm();return None;
        }
        const Vec local=localPoint(frame.world,frame.fingertip);
        // A sample deeper than the bounded tracking volume is a tunnel, not a press.
        // It cannot return through the face and activate without a fresh approach.
        if(!finite(local)||local.z<buttonLocal(Reload).z-MaxPressDepth) {
            disarm();return None;
        }
        if(!armed) {
            // Initial overlap and continued depression cannot activate, including
            // after opening/closing a panel. Retract in front of the whole cuff.
            if(now>=cooldownUntil&&local.z>=buttonLocal(Weapons).z+ReleaseClearance)armed=true;
            candidate=None;previousLocal=local;havePrevious=armed;return None;
        }
        int pick=None;float nearest=1e30f;
        for(int i=0;i<4;i++) {
            const auto delta=local-buttonLocal(i);
            const float distance2=delta.x*delta.x+delta.y*delta.y;
            const float exitRadius=buttonRadius(i)+.35f*Cm;
            if(distance2>exitRadius*exitRadius)approached[i]=false;
            // Observe this fingertip above this cap before accepting penetration.
            // Lateral entry or emergence through the back does not establish intent.
            else if(delta.z>=.05f*Cm)approached[i]=true;
            auto press=physicalPress(local,i);
            if(press.radial&&press.signedDepth>MaxPressDepth){disarm();return None;}
            if(!havePrevious||!approached[i]||!press.valid||press.signedDepth<PressThreshold)continue;
            const auto previousDelta=previousLocal-buttonLocal(i);
            const float previousDepth=-previousDelta.z;
            // One physical front-to-pressed crossing. No dwell in a sub-frame
            // depth band: a natural tap has no real collision stopping its finger.
            if(previousDepth>=PressThreshold||press.signedDepth<=previousDepth)continue;
            const float fraction=(PressThreshold-previousDepth)/(press.signedDepth-previousDepth);
            const auto crossing=previousDelta+(delta-previousDelta)*fraction;
            const float radius=buttonRadius(i);
            if(crossing.x*crossing.x+crossing.y*crossing.y<=radius*radius&&distance2<nearest) {
                pick=i;nearest=distance2;
            }
        }
        previousLocal=local;havePrevious=true;
        if(pick==None){candidate=None;candidateSince=0;return None;}
        disarm();cooldownUntil=now+400;return pick;
    }
};
}
