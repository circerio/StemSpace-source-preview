# Offline validation summary and remaining gates

This is a sanitized summary of completed private qualification, not a new test run or publication of raw research evidence. The delivered DSP source/configuration bytes match the qualified beta.2 source snapshot; hashes are listed in `SOURCE_MANIFEST.json`. The source-only export was inspected and archived, not newly built, installed or played.

## Completed within the recorded scope

| Gate | Result and scope |
|---|---|
| A1 product regression | PASS under the adopted O3/Z2 v1.0 contract with fixed toolchain/flags, floating-point environment, ORT/model and inputs. Earlier full-equality traces include 1,620 records / 720 chunks and a further 295 records spanning three rates. |
| A2 strict formula implementation | PASS: 619,104 samples, actual product mixer in an isolated strict floating-point diagnostic build versus independent Python, raw float32/float64 bits including signed zero. This is not a Release/Python bitwise claim. |
| A2 Release error bound | PASS: 617,056 samples inside independently frozen formulas and prederived forward-error bounds. Analysis domain: finite inputs, no overflow, round-to-nearest-even, FTZ/DAZ off, gradual underflow, absolute input at most 2. Outside that domain is not proved. |
| Structure and coefficient sensitivity | PASS for fixed independent impulse/cross-channel fixtures: original L/R geometry, Center from original Mid, Drums/Other Side/Rear routes, zero coupling from independent Bass/Vocals inputs and zero LFE. |
| Streaming | PASS offline, including 44.1/48/96 kHz, irregular chunks, short prebuffer fragments, partial hops, drain, separate EOF/END_OF_TRACK, seek/flush/rate switches, mono/native multichannel bypass and worker-creation failure propagation. A further 105-record trace supplements the earlier traces. |
| Non-finite model output injection | PASS for the exercised failure/fallback cases; not a claim of mathematical validity for arbitrary NaN/Inf inputs. |
| Qualified DSP Release x64 build/privacy | PASS for the previously verified binary and source/build-input lineage: CodeView retains its GUID/age with a PDB basename, and reviewed private-path scanning passed. No DLL or PDB is included in this release. |

A1 compares raw PCM plus layout/mask, sample rate, chunk count/size/order, input/flush/EOF/END_OF_TRACK, frame count, drain and format-switch behavior. There is no epsilon/ULP allowance and no short-fragment exception. Z2 applies only to BL/BR/SL/SR samples where both versions are IEEE zero and only the sign bit differs, with verified per-case causality from removal of an old research-only term. All 51 observed sign-only differences were retained and individually investigated; zeros were not normalized before the raw comparison. FL/FR/C/LFE, nonzero/subnormal/NaN/Inf outputs and metadata are outside this exception. The strict diagnostic's sign results do not define a universal future `/fp:fast` zero-sign contract. A2 bounds never serve as A1 tolerances.

The first independent Python Release-bitwise oracle was not qualified: it assumed an evaluation/rounding model that differed from the observed optimized Release evaluation. That failed comparison was retained privately. O3 separates product regression, strict formula implementation and bounded Release numerical correctness instead of relabeling that failure as an exact match. This summary does not erase historical failures or assert that old installed research DLLs have a fully proved source/build lineage.

## Not accepted by this source release

- Real foobar host load, error/UI delivery and runtime behavior.
- Isolated portable installation, upgrade, settings/volume/DSP-chain/output-selection side effects, uninstall/rollback, and native meter v1/v3 host compatibility/lifecycle.
- Real Windows Spatial endpoint/renderer activation, full lifecycle and hardware/LFE behavior. Spatial Output is excluded pending rights and release acceptance.
- Listening quality or end-user usability, package/public binary distribution clearance, or model redistribution.

The overall P4 validation status remains **BLOCKED**. This public source preview is separately authorized and does not mark unrun tests PASS, adopt a new P4/P5 boundary, or approve a binary release. Reported offline PASS is confined to preserved inputs, program/toolchain identities and stated assumptions; it is not a general safety or perceptual-quality guarantee. Raw test audio, fixtures, model, private logs, PDBs and research reports are intentionally not published.
