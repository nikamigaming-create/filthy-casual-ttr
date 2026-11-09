# Third-party notices

The OpenXR 1.1.60 headers are from KhronosGroup/OpenXR-SDK, source archive
KhronosGroup-OpenXR-SDK-4ce2937. Original copyright and SPDX notices are retained
under Apache-2.0 OR MIT; both license texts are in third_party/openxr/LICENSES.
Upstream: https://github.com/KhronosGroup/OpenXR-SDK/tree/release-1.1.60

Fallout: New Vegas, Tale of Two Wastelands, KUPOkinz's A Tale of Two Realities,
xNVSE and JIP LN NVSE are external dependencies. Their game content and binaries
are not distributed here. The addon uses a minimal native plugin ABI declaration
and guarded integration points; it does not include the base mod's source.
The development simulator and private diagnostics are not in this public snapshot.
Original addon code is covered by LICENSE.

## Optional standalone pool addon

Pool Tables Of The Mojave v4 by Luke2172 remains a separate required download:
https://www.nexusmods.com/newvegas/mods/100097
No original pool assets, ESP or completed upstream-derived scripts are included.
Prepare-Pool.ps1 reads the user's verified original installation to generate
three local integration scripts. These local generated outputs are excluded
from source and release archives. Animation tracks are newly authored against
the locally read game bind rig; no retail skeleton or animation is included.
