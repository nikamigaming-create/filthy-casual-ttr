# Third-party code

## OpenXR

The headers in `third_party/openxr/include/openxr` are the complete OpenXR 1.1.60
header set from the existing local SDK checkout, identified by the source archive
as `KhronosGroup-OpenXR-SDK-4ce2937`. Both addon and simulator use this same set.
Copyright and SPDX notices are retained (Apache-2.0 OR MIT), with license texts
in `third_party/openxr/LICENSES`.
Upstream: https://github.com/KhronosGroup/OpenXR-SDK/tree/release-1.1.60

## bvr-xrsim32

`xrsim` is a locally modified import from
https://github.com/VR-Stereo-Hub/bioshock-trilogy-vr/tree/main/src/tools/xrsim
under the retained `xrsim/LICENSE`. The exact upstream commit was not recorded
during initial setup. Local changes include per-hand actions, quad/view matrix
layout, and capture diagnostics. The first Git checkpoint preserves this state.

Fallout: New Vegas, TTW, TTWVR, xNVSE, and JIP LN NVSE are external runtime
dependencies. Their binaries and game content are not included in this repository.

## Quest Touch Plus controls editor

`assets/controls/left.mesh`, `right.mesh` and their PNG textures derive from
WebXR Input Profiles Touch Plus GLBs under the retained Amazon MIT license.
`provenance.json` records the exact source model hashes and export counts.
The D3D11 model loader/shaders in `controls_models.hpp` are adapted from MIT
MGS5VR `src/controls_panel_renderer.cpp`; both license files ship with the assets.
The RobCo-style terminal artwork was generated with the built-in image_gen tool
for this addon. It is not extracted from Fallout. See the asset provenance JSON.

## Controller expansion

WebXR Input Profiles assets v1.0.20: MIT, copyright Amazon. Controller model source URLs and SHA-256 hashes are in `assets/controls/controller-models-provenance.json`; license in `assets/controls/LICENSE-webxr-controllers.md`. Steam Frame uses an original labelled layout diagram, not a redistributed Valve model.
