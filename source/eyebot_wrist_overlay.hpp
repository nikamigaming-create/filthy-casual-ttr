#pragma once

#include "eyebot/wrist_frame.hpp"

// The XR layer identifies the stock wrist surface for input focus. During
// an Eyebot page the original 3D CRT material provides the display pixels.
namespace eyebot_wrist_overlay {
inline XrSpace headSpace = XR_NULL_HANDLE;
inline PFN_xrEndFrame forward = nullptr;

inline void cleanup() {
    if (headSpace) overlay::destroySpace(headSpace);
    headSpace = XR_NULL_HANDLE;
}

inline bool wristSurface(const XrCompositionLayerQuad& quad) {
    return std::abs(quad.size.width - .0816f) <= .0025f &&
           std::abs(quad.size.height - .0627f) <= .0025f;
}

inline unsigned interest(XrSession session, XrTime time, const XrCompositionLayerQuad& quad) {
    if (!height_layer::locateSpace) return 0;
    const auto p = quad.pose.position;
    if (gesture_input::aim[0]) {
        XrSpaceLocation hand{XR_TYPE_SPACE_LOCATION};
        if (XR_SUCCEEDED(height_layer::locateSpace(gesture_input::aim[0],quad.space,time,&hand)) &&
            (hand.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT)) {
            const auto h = hand.pose.position;
            if ((p.x-h.x)*(p.x-h.x)+(p.y-h.y)*(p.y-h.y)+(p.z-h.z)*(p.z-h.z) > .34f*.34f) return 0;
        }
    }
    if (!headSpace && overlay::createSpace) {
        XrReferenceSpaceCreateInfo info{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
        info.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
        info.poseInReferenceSpace.orientation.w = 1.f;
        if (XR_FAILED(overlay::createSpace(session,&info,&headSpace))) return 0;
    }
    if (!headSpace) return 0;
    XrSpaceLocation head{XR_TYPE_SPACE_LOCATION};
    if (XR_FAILED(height_layer::locateSpace(headSpace,quad.space,time,&head)) ||
        !(head.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) ||
        !(head.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT)) return 0;
    using namespace eyebot::wrist_frame;
    const auto eye = head.pose;
    const auto headForward = controls_overlay::rotate(eye.orientation,{0,0,-1});
    const XrVector3f where{p.x-eye.position.x,p.y-eye.position.y,p.z-eye.position.z};
    const float d2 = where.x*where.x + where.y*where.y + where.z*where.z;
    if (d2 < .09f*.09f || d2 > .78f*.78f) return 0;
    return Visible | (headForward.x*where.x + headForward.y*where.y + headForward.z*where.z >
        .68f*std::sqrt(d2) ? Focused : 0);
}

inline unsigned pointAtFilthy(const XrCompositionLayerQuad& quad,XrTime time,unsigned layout){
    using namespace eyebot::wrist_frame;
    const auto width=quad.size.width,height=quad.size.height;
    if(width<.55f||width>3.f||height<.4f||height>2.4f||width/height<1.2f||width/height>1.4f||
        !gesture_input::aim[1]||!height_layer::locateSpace)return false;
    XrSpaceLocation hand{XR_TYPE_SPACE_LOCATION};
    constexpr auto valid=XR_SPACE_LOCATION_POSITION_VALID_BIT|XR_SPACE_LOCATION_ORIENTATION_VALID_BIT;
    if(XR_FAILED(height_layer::locateSpace(gesture_input::aim[1],quad.space,time,&hand))||
        (hand.locationFlags&valid)!=valid)return false;
    const auto p=quad.pose.position;
    const auto q=quad.pose.orientation;
    const XrQuaternionf inverse{-q.x,-q.y,-q.z,q.w};
    const auto from=hand.pose.position;
    const auto origin=controls_overlay::rotate(inverse,{from.x-p.x,from.y-p.y,from.z-p.z});
    const auto direction=controls_overlay::rotate(inverse,controls_overlay::rotate(hand.pose.orientation,{0,0,-1}));
    if(origin.z<=0||direction.z>=-.02f)return false;
    const float t=-origin.z/direction.z;
    const float u=.5f+(origin.x+t*direction.x)/width;
    const float v=.5f-(origin.y+t*direction.y)/height;
    return fullMenuPointer(u,v,uint8_t(layout),bool(layout&0x100));
}

inline XrResult XRAPI_CALL submit(XrSession session, const XrFrameEndInfo* frame) {
    auto module = GetModuleHandleW(L"FilthyCasualNativeSettings.dll");
    if (!module) module = GetModuleHandleW(L"RobCoEyebot.dll");
    using NativeCrt = unsigned (__cdecl*)(unsigned);
    auto bridge = module ? reinterpret_cast<NativeCrt>(
        GetProcAddress(module, "FC_EyebotNativeCrt")) : nullptr;
    using Pointer = void (__cdecl*)(unsigned);
    auto pointer=module?reinterpret_cast<Pointer>(GetProcAddress(module,"FC_FilthyPointed")):nullptr;
    if (!bridge || !frame || !frame->layerCount || frame->layerCount > 32) return forward(session,frame);
    using Layout=unsigned (__cdecl*)();
    auto readLayout=reinterpret_cast<Layout>(GetProcAddress(module,"FC_FilthyPointerLayout"));
    const unsigned layout=readLayout?readLayout():0x7e;
    unsigned pointed=0;
    uint32_t wristLayers = 0;
    unsigned found = 0;
    for (uint32_t i = 0; i != frame->layerCount; ++i) {
        if (frame->layers[i]->type != XR_TYPE_COMPOSITION_LAYER_QUAD) continue;
        const auto& quad = *reinterpret_cast<const XrCompositionLayerQuad*>(frame->layers[i]);
        if(pointer&&!pointed)pointed=pointAtFilthy(quad,frame->displayTime,layout);
        if (!wristSurface(quad)) continue;
        wristLayers |= uint32_t(1) << i;
        found |= interest(session,frame->displayTime,quad);
    }
    if(pointer)pointer(pointed);
    const auto ready = bridge(found);
    if (!wristLayers || !ready) return forward(session,frame);
    // Focus controls interaction. Visibility of the stock CRT follows page ownership.
    const XrCompositionLayerBaseHeader* layers[32];
    auto next = *frame;
    next.layerCount = 0;
    for (uint32_t i = 0; i < frame->layerCount; ++i)
        if (!(wristLayers & (uint32_t(1) << i))) layers[next.layerCount++] = frame->layers[i];
    next.layers = layers;
    return forward(session,&next);
}
}
