# Source scope and provenance

Release `v0.1.0-beta.2-source-preview` is a fresh public root snapshot, not a push of any private development branch or its history. Product source and build properties are exported byte-for-byte from the qualified beta.2 DSP snapshot. Public-facing documentation, ignore/attribute rules and manifests are prepared separately; no audio algorithm, compiler setting, runtime contract or installed component was changed for publication.

## Included and rights basis

| Scope | Rights and attribution |
|---|---|
| DSP integration, worker/batching, StableMix C and native meter publisher | Existing project MIT scope, copyright circerio. On 2026-10-24 the maintainer explicitly confirmed rights/permission for identifiable human contributions and the disclosed third-party reference scope for DSP, publisher and local shims. This is a current maintainer confirmation, not a reconstructed historical signature or a claim that AI assistance alone determines legal authorship. |
| Runtime, constants, sample-rate adapter and contract interface | Adapted StemgenRT, copyright Axel Delafosse, MIT. Fixed upstream revision `61df8f4aa1555ef110308d01ea92b54ace770979`; retain its full original license and distinguish local adaptations. |
| Local `juce_*` shim headers | Confirmed project MIT scope, small local C++/Windows API compatibility code; no full JUCE implementation is distributed. |
| SDK-style interface/PCH boilerplate | Applicable foobar2000 SDK notice preserved; no SDK tree or linked library is shipped. |
| License texts | Full originals retained under their original terms; not relicensed by the project MIT license. |

Pinned upstream paths for the adapted material are `plugin/source/OnnxRuntime.cpp`, `plugin/source/StreamingSampleRateAdapter.cpp`, the corresponding `plugin/include/StemgenRT` headers and `cmake/QualifiedModelContract.h.in`. `Constants.h` differs only in raw line endings. Local runtime changes add a NOMINMAX guard and optional spinning-control parameter; the production DSP passes `false`. The adapter retains local 8 kHz minimum/low-Nyquist transition support. The contract header expands the fixed upstream interface/metadata template. It contains shapes, tensor names, compatibility metadata and hashes, not model weights or training examples. Model metadata identifiers containing the word `private` are upstream contract identifiers, not access tokens, local paths or private research files.

## Explicit exclusions

All Windows Spatial Output implementation, Engine/Ring/backend/notification/telemetry sources, its tests and DLLs are excluded; their ownership/attribution and release acceptance remain separately unresolved. The standalone meter viewer is excluded. No conclusion about those sources' ownership is implied by this preview.

Also excluded: inherited Git objects/branches/tags, old drafts/assets, SDK/vendor trees, third-party binaries, model/checkpoint weights or LFS pointers, training data, audio/PCM fixtures, private PDBs, user profiles/SQLite/settings, raw research evidence and private release reports. External runtime, SDK and toolchain prerequisites are documented rather than copied into the tree. No unreviewed foreign diagnostic files are included.

The repository contains a complete public file allowlist in `PUBLIC_FILE_MANIFEST.json` (excluding that manifest itself). `SOURCE_MANIFEST.json` gives the byte hashes and rights basis for the exact qualified source/configuration exports. The release source ZIP is assembled from this new public Git tree, with no `.git` directory or binary payload.
