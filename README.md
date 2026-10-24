# StemSpace v0.1.0-beta.2 — Experimental Source Preview

StemSpace is an experimental foobar2000 AI-assisted stereo-to-7.1 DSP for Windows x64.
This release publishes reviewed DSP source code and technical notes. **It is not an installable or self-contained AI upmixer.**

No model weights, ONNX Runtime DLL, compiled component, installer, private PDB, audio, user configuration or private research evidence is included. AI Stereo→7.1 requires separately and lawfully obtained compatible model and runtime prerequisites. Their availability and redistribution rights are not promised by this release. Without a ready model/runtime the DSP passes input through; that is not working AI upmixing.

## Design

- StableMix C retains the original stereo Mid/Side front image. Center comes only from original Mid; Vocals/Bass do not independently steer positioning. Drums/Other contribute to fixed Side/Rear routes. AI LFE is zero.
- Native non-stereo input bypasses the AI path, including mono, 5.1 and 7.1.
- The existing sample-rate adapter bridges the host rate and the fixed 44.1 kHz model clock. Offline regression coverage includes 44.1, 48 and 96 kHz; this is not a claim that every accepted rate has equivalent coverage.
- Scheduling retains a dedicated high-priority non-MMCSS worker, ORT intra-op 1 with spinning disabled, one batched 7.1 output per producing input callback, and a 250 ms start/flush prebuffer. Drained output can also be emitted at end of playback.

See [StableMix C](docs/STABLEMIX_C.md), [building](BUILDING.md), [offline validation and limits](docs/OFFLINE_VALIDATION.md), and [source provenance](docs/SOURCE_SCOPE_AND_PROVENANCE.md).

## Status and exclusions

Mixer and streaming have passed the documented offline qualification within fixed inputs, toolchain and floating-point assumptions. These results do not establish listening quality, universal numerical equivalence, portable installation safety, or real foobar host/Spatial endpoint acceptance. **The full release-validation gate remains BLOCKED.** No new build or playback was performed to publish this snapshot.

Windows Spatial Output source and binaries are not included. They require separate source-rights and release acceptance work before a future delivery. This DSP does not depend on that output component. No third-party Spatial Output DLL is included.

The native meter publisher remains part of the DSP source. The standalone viewer is excluded from this preview. Publisher mappings use a Windows session-local namespace; running another instance in the same session is not safe isolation. See the limitations in the build notes before any future host testing.

## License

The confirmed project-owned DSP, meter-publisher and local compatibility-shim scope uses the existing [MIT license](LICENSE), copyright 2026 circerio. Adapted StemgenRT code retains Axel Delafosse's MIT notice. SDK/interface and other applicable notices remain separate; see [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). The project MIT grant does not grant model, teacher, dataset, SDK/toolchain or third-party binary rights.

This repository starts with a fresh source-only history. It contains no inherited research branches, old binary assets or private repository history. [SOURCE_MANIFEST.json](SOURCE_MANIFEST.json) records the reviewed source snapshot hashes; [PUBLIC_FILE_MANIFEST.json](PUBLIC_FILE_MANIFEST.json) lists the complete public payload except itself.
