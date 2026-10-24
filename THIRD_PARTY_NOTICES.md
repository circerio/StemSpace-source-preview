# Third-party notices and source scope

The root MIT license covers confirmed project-owned contributions only. It does not replace third-party notices or establish rights in model weights, training data, SDK/toolchain binaries or excluded Spatial Output code. No endorsement by upstream authors is implied.

## StemgenRT — Axel Delafosse

The runtime, constants, sample-rate adapter and model-contract interface under `src/stemgen_core` derive from [StemgenRT](https://github.com/sweetspotsoundsystem/stemgen-rt) at revision `61df8f4aa1555ef110308d01ea92b54ace770979`. Preserve the complete [StemgenRT MIT license](third_party/licenses/StemgenRT-MIT.txt), copyright 2026 Axel Delafosse. These are adapted sources, not an unmodified upstream release.

Local changes retained from the qualified source include the `NOMINMAX` guard, the optional spinning-control argument (the DSP passes false), and low-rate sample-rate-adapter support with transition edges following the lower Nyquist frequency. `QualifiedModelContract.h` is the generated fixed interface/metadata form of upstream's contract template; no weights or checkpoint tensors are present. Constants match the pinned upstream after diagnostic line-ending normalization. Exported product sources themselves retain their exact qualified bytes. See [provenance](docs/SOURCE_SCOPE_AND_PROVENANCE.md).

`src/stemgen_core/juce_core/juce_core.h` and `juce_cryptography/juce_cryptography.h` are small local compatibility implementations over standard C++/Windows APIs, not copies or a distribution of the full JUCE framework. Their disclosed source and rights scope were confirmed by the maintainer for this release. No JUCE framework, JUCE/VST3 project, training pipeline, model, teacher or dataset is included.

## foobar2000 SDK / component client / pfc

The DSP uses the external foobar2000 2.x SDK, qualified version 2026-10-01. The small SDK-style PCH/interface boilerplate and any derived interface usage retain the full [SDK notice](third_party/licenses/foobar2000-SDK.txt), copyright Peter Pawlowski. Component-client and shared import-library references are part of the external SDK. The SDK and host DLL are not distributed here.

The project links an externally prepared pfc library. Preserve the separate [pfc license](third_party/licenses/pfc-license.txt); it is not replaced by the SDK notice or the project MIT grant. No pfc implementation is copied into this source preview. No libPPUI implementation or binary is shipped, and libPPUI was not an observed linked dependency of the qualified DSP build. Additional dependencies used by a downstream SDK build retain their own terms.

## Microsoft ONNX Runtime

The integration consumes external ORT C API headers. Preserve the [Microsoft MIT notice](third_party/licenses/ONNXRuntime-MIT.txt). The [complete upstream runtime third-party notices](third_party/licenses/ONNXRuntime-ThirdPartyNotices.txt) are supplied unabridged for reference to the qualified external 1.26.0 runtime; listing those components does not assert that each is linked into this preview or the Windows CPU binary.

No ORT header, library or DLL is shipped. Any later runtime redistribution must independently satisfy its applicable source/notice/license obligations. A narrowly scoped privacy exception for previously verified upstream CI paths is not a general binary distribution grant. Official source/license references: [ORT 1.26.0 license](https://github.com/microsoft/onnxruntime/blob/v1.26.0/LICENSE), [notices](https://github.com/microsoft/onnxruntime/blob/v1.26.0/ThirdPartyNotices.txt).

## External development and system dependencies

Windows SDK headers/import libraries, MSVC tools and runtime, UCRT, and foobar2000 are external prerequisites under their respective terms. None of their binaries/import libraries, redistributable installers or SDK distributions is in this repository or release archive. Source publication does not grant entitlement to use or redistribute those products. Separately built binary redistribution needs its own review.
