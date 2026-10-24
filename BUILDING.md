# Building the source preview

This is developer source, not an installation procedure. No compiled DLL or installable package is supplied. Building successfully does not resolve the model/runtime rights or unfinished host/portable acceptance gates.

## External prerequisites

1. A licensed Windows x64 development environment with Visual Studio/MSBuild, MSVC v143, C++20 and a suitable Windows SDK. The offline-qualified compiler was 19.38.33145 (toolset 14.38.33130), Windows SDK 10.0.26100.0. A different toolchain is not automatically covered by the qualification.
2. The [official foobar2000 SDK](https://www.foobar2000.org/SDK), version 2026-10-01. Obtain it separately and retain its root, pfc and any other applicable notices. Prepare compatible Release x64 `pfc.lib`, `foobar2000_SDK.lib`, `foobar2000_component_client.lib`, and the SDK's `shared-x64.lib` in a separate development location under the SDK's terms. None are shipped here. The SDK/component-client/pfc sources and host `shared.dll` are not bundled.
3. Official [ONNX Runtime 1.26.0](https://github.com/microsoft/onnxruntime/releases/tag/v1.26.0) Windows x64 CPU development headers. The project consumes `include/onnxruntime_c_api.h` from an external package root and resolves the DLL dynamically; no ORT import library is required by this project. ORT binaries and their compiled-dependency distribution obligations remain outside this source release.

Open a VS x64 developer shell. Set the two local variables below to separately prepared dependency roots; do not commit machine-specific values:

```powershell
# Set $sdkRoot and $ortRoot to your separately obtained dependency locations.
msbuild src/foo_dsp_stemgen_spatial/foo_dsp_stemgen_spatial.vcxproj /m:1 /t:Build /p:Configuration=Release /p:Platform=x64 "/p:StemSpaceSdkRoot=$sdkRoot" "/p:StemSpaceOrtRoot=$ortRoot" /p:VCToolsVersion=14.38.33130 /p:WindowsTargetPlatformVersion=10.0.26100.0
```

The project checks dependency files before building; it never builds or writes dependency projects. Outputs go to `build/Release/x64/`, intermediates to `build/obj/dsp/Release/x64/`. There is no post-build installation, model download, foobar launch or audio playback step. The project retains `/MD`, `/fp:fast`, optimization settings, private debug information and `/PDBALTPATH:%_PDB%`. A private PDB is still sensitive even when the DLL's CodeView path is only a basename. Keep build outputs and PDBs private until separately reviewed.

## Runtime is not supplied

The intended DSP filename is `foo_stemspace.dll`; version is `0.1.0-beta.2`, DSP GUID `F60FE095-9672-4D5C-A18F-B635F4978CA9`.

The source expects a compatible `onnxruntime.dll` and the exact contract-compatible `model.onnx` adjacent to the component DLL. A random stereo separation model is not compatible. `QualifiedModelContract.h` contains interface/metadata and SHA checks, not model weights. The previously qualified CPU runtime SHA256 is `B2BA7CA16E0E4FE71AD5148744AB885A2F5809E52A0C3DE4D9BA3853A03977F9`. A hash identifies bytes; it is not redistribution permission. This preview supplies neither file and does not direct automatic acquisition of weights.

Model/teacher/training-data rights remain unresolved. Do not infer a weight redistribution or commercial-use grant from StemgenRT's code MIT license, and do not assume dataset conditions automatically transfer to weights. Obtain the applicable rights before use or distribution. There is no model downloader or alternative model policy in this release.

If initialization fails, the DSP bypasses rather than providing AI Stereo→7.1. DLL load failure can latch for the module lifetime; adding prerequisites during playback is not a supported recovery guarantee. A future runtime deployment requires separately reviewed installation/restart handling, compatible x64 MSVC runtime and OS UCRT. Do not copy Windows system DLLs, host `shared.dll`, or arbitrary runtime libraries into a component package.

## Host and meter caution

This preview provides no recommendation to replace an installed component. Portable load/upgrade/rollback, actual host errors/UI, and real Spatial endpoint acceptance remain uncompleted. Host testing requires a separately qualified isolated environment with no playback. The meter publisher uses `Local\StemgenRTSpatial71Meter` (snapshot v3, 160 bytes) and `Local\StemgenRTSpatial71MeterRing` (timeline v1, 96-byte entries). Another portable directory in the same Windows session can share these mappings. A matching viewer is not distributed, and v3 timeline readers are not compatible with the v1 timeline. Do not launch parallel production/test publishers in the same session.
