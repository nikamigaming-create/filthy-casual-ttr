# Source and builds

The player download is linked from the [repository front page](../README.md). This folder contains the add-on source, original build inputs, and development tools.

Use Visual Studio Build Tools with the C++ x86 compiler. From this directory, run `build.cmd` for the core, OpenXR input layer, and optional support logger. The vehicle, device settings, and holster modules have separate build commands.

```bat
build.cmd
vehicle-vr\build.cmd
eyebot\build.cmd
build-physical-workshop.cmd
```

The output DLLs go under `build\`. Keep generated assets and local QA outputs outside version control. The source snapshot can be built independently. A matching DLL checksum requires the original build inputs and toolchain for that release.

On the build machine, `package.ps1` verifies the archived RC21 payload, adds the required vehicle scripts, and creates a standard MO2 archive. Supply `-Rc21Archive`, `-OutputZip`, and `-Version`. Use `-VehicleDll` to supply the exact build being tested.

[Current release validation](VALIDATION.md).
