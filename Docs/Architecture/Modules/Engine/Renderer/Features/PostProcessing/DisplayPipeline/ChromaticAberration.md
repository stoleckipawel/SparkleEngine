# Renderer Chromatic Aberration

**Status:** first-release target feature dossier plus current source-backed absence; not implementation or acceptance evidence

**Verified:** 2026-09-06 against source revision `d236da11`; inspected Renderer/shader/view paths are unchanged from the earlier `8414b5dc` audit

**Scope:** `REN-POST-12`; intentional wavelength/channel-dependent lens distortion as a post-processing effect

**Parent family:** [Post Processing](../README.md)

**First-release admission:** `FCR-REN-25`; implementation phase [`DSP-6`](../../../FirstRelease/DisplayAndReconstruction.md#dsp-6--chromatic-aberration)

**Current readiness:** **0/100** — admitted and `Blocked`; no lens/channel-distortion controls, pass, shader, selector, or editor route was found. See [Current Feature Readiness](../../../../../../../Acceptance/CurrentReadiness.md#renderer).

## At A Glance

| Question | Current answer |
| --- | --- |
| Is there an intentional chromatic-aberration effect? | No selector, setting, pass, shader, lens model, or editor control exists. |
| Are colored reconstruction/filtering fringes the feature? | No. They are artifacts unless produced by a named lens model with authored controls. |
| Where does the target effect live? | After target-specific tone/gamut mapping and before final encoding/UI, on the output-resolution target-linear image. |
| What is the main design cost? | The effect needs bounded edge sampling, stable center/aspect behavior, extra texture reads, and resolution-independent tuning without contaminating alpha or UI. |

The admitted implementation must make zero strength an exact identity transform and keep artifacts from being mislabeled as deliberate output. The retained negative boundary protects both current feature claims and image-quality bug classification until that implementation exists.

## Current Capability

Chromatic aberration was not found. Sparkle has no per-view lens/aberration settings, selector, pass, shader, radial or spectral distortion model, center/falloff controls, edge sampling policy, history, diagnostic mode, or editor authoring surface.

Color fringes caused by reconstruction, filtering, motion, texture sampling, or display encoding are artifacts, not an implemented chromatic-aberration feature. No neighboring post stage may be advertised as this effect.

## First-Release Target Contract

The first-release effect is one View-owned, output-resolution lens-fringe pass after tone mapping and grading but before output encoding and UI. It uses normalized center and start offset plus a strength expressed in pixels at a 1080-line reference height; scaling by output height keeps authored intent stable across resolution. RGB samples separate radially with bounded clamp-to-edge sampling. Unreal's [intensity and start-offset model](https://dev.epicgames.com/documentation/unreal-engine/post-process-effects-in-unreal-engine#chromaticaberration) is the reviewed product precedent, not a shader implementation to copy.

The admitted scope must define and implement:

- the fixed post-tone/pre-encode position, with raw input/output capture identity and UI excluded from distortion;
- the radial RGB channel-offset model, reference-height units, viewport center/aspect behavior, bilinear sampling, bounded edge clamp, alpha preservation, and one quality mode;
- per-camera/per-viewport settings, defaults, serialization, editor controls, requested/active diagnostics, and disable/identity behavior;
- extent/resize/history behavior, compute or graphics pass ownership, backend requirements, cost, and controlled failure;
- reference patterns for zero-strength identity, center stability, radial symmetry, edge behavior, temporal stability, and resolution independence.

Physical lens calibration, spectral rendering, anamorphic models, per-channel author curves, guard-band expansion, history, local volumes, and multiple quality modes are non-goals for `v0.1.0`. Until implementation exists, the admitted feature remains unavailable and `Blocked`, not Experimental.

## First-Release Acceptance Criteria

- `AC-CHR-01` — zero strength is an exact identity and omits the pass; disabled state does not allocate history or persistent GPU data.
- `AC-CHR-02` — the radial channel-offset equation matches a CPU reference for center, axes, diagonals, aspect ratios, start offsets, strengths, and edge coordinates within declared filtering tolerance.
- `AC-CHR-03` — strength scales from the 1080-line reference to output height, so matched normalized patterns have the same authored displacement within tolerance at every admitted resolution.
- `AC-CHR-04` — the pass consumes tone-mapped display-linear color after grading and before encoding/UI exactly once; alpha and viewport/product identity are preserved.
- `AC-CHR-05` — per-view requested/resolved/active state, persistence, live edit, two-viewport isolation, resize/DPI, capture metadata, and invalid values behave deterministically.
- `AC-CHR-06` — non-finite, negative/out-of-range, zero-extent, missing-input, stale-generation, and resource/pipeline failures reject or produce the documented identity safe state without stale or partial output.
- `AC-CHR-07` — D3D12/Vulkan raw results, temporal stability, package operation, pass cost, and memory satisfy declared release tolerances and budgets.
- `AC-CHR-08` — ordinary reconstruction/filtering fringes remain defects; excluded physical/spectral/local-volume/guard-band/history variants are not claimed.

## First-Release Failure Modes And Checks

| Failure ID | Controlled setup | Required safe behavior | Check |
| --- | --- | --- | --- |
| `FM-CHR-01` | invalid/non-finite strength, center, or start offset | reject or clamp exactly as documented before active publication; identify the field | `CHK-CHR-01`, `CHK-CHR-04` |
| `FM-CHR-02` | zero extent, missing input, stale generation, or resize during execution | no out-of-bounds/stale sampling; identity/refusal and rebuild are explicit | `CHK-CHR-03`, `CHK-CHR-04` |
| `FM-CHR-03` | effect runs in the wrong color domain/order or distorts UI/alpha | topology/raw-product oracle fails the candidate | `CHK-CHR-02` |
| `FM-CHR-04` | high strength reaches image edges | all sample coordinates remain bounded and deterministic; no NaN, wrap, or memory fault | `CHK-CHR-01`, `CHK-CHR-03` |
| `FM-CHR-05` | backend/package result or cost differs materially | block rather than silently disable or relabel artifacts as the feature | `CHK-CHR-05` |

| Check | Exercise and oracle | Covers |
| --- | --- | --- |
| `CHK-CHR-01` | CPU-versus-shader radial pattern over centers, axes, diagonals, strengths, offsets, aspects, and edges | `AC-CHR-01`, `AC-CHR-02`; `FM-CHR-01`, `FM-CHR-04` |
| `CHK-CHR-02` | graph/domain audit and raw pre/post/encoded/UI captures with alpha sentinel | `AC-CHR-04`; `FM-CHR-03` |
| `CHK-CHR-03` | matched patterns at admitted resolutions plus resize, DPI, edge, zero-extent, and temporal camera cases | `AC-CHR-03`, `AC-CHR-07`; `FM-CHR-02`, `FM-CHR-04` |
| `CHK-CHR-04` | settings round trip, two viewports, invalid input, missing resource/pipeline, generation failure and recovery | `AC-CHR-05`, `AC-CHR-06`; `FM-CHR-01`, `FM-CHR-02` |
| `CHK-CHR-05` | paired D3D12/Vulkan raw comparison, package route, timing/memory and artifact-classification review | `AC-CHR-07`, `AC-CHR-08`; `FM-CHR-05` |

## Evidence And Source Audit

- `REN-E27` owns the negative source/build/selector/pass/shader/asset/editor/documentation audit for chromatic aberration.
- No runtime test is implied by this source-only absence finding.
- Adjacent source routes inspected: [`PostProcessing.cpp`](../../../../../../../../Engine/Renderer/Private/Passes/PostProcessing/PostProcessing.cpp), [`Presentation.cpp`](../../../../../../../../Engine/Renderer/Private/Passes/Presentation/Presentation.cpp), and [`ViewportDisplaySettings.h`](../../../../../../../../Engine/Renderer/Private/View/ViewportDisplaySettings.h).

### Current Negative Acceptance

- `AC-CHR-NEG-01` — no selector, view/camera setting, lens model, shader/pass, history, diagnostic, editor control, or package claim advertises chromatic aberration.
- `AC-CHR-NEG-02` — reconstruction, filtering, motion, sampling, and encoding fringes remain classified as defects/artifacts rather than intentional feature output.

`FM-CHR-NEG-01` occurs when a reachable aberration-like control/path lacks the complete owner/result contract or an artifact is advertised as support. `CHK-CHR-NEG-01`/`REN-E27` covers `AC-CHR-NEG-01`, `AC-CHR-NEG-02`, and `FM-CHR-NEG-01` by auditing source, CMake, shaders, settings, frame graph, view/editor UI, diagnostics, packages, and documentation; any unmatched result fails the negative contract.
