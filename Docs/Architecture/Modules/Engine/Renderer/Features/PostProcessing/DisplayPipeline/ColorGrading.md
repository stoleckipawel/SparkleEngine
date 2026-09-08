# Renderer Color Grading

**Status:** first-release target feature dossier plus current source-backed absence; not implementation or acceptance evidence

**Verified:** 2026-09-06 against source revision `d236da11`; inspected Renderer/shader/asset paths are unchanged from the earlier `8414b5dc` audit

**Scope:** `REN-POST-11`; artistic and technical color adjustments distinct from exposure, tone mapping, and output encoding

**Parent family:** [Post Processing](../README.md)

**First-release admission:** `FCR-REN-24`; implementation phase [`DSP-5`](../../../FirstRelease/DisplayAndReconstruction.md#dsp-5--color-grading)

**Current readiness:** **0/100** — admitted and `Blocked`; no grading controls, transforms, LUT resource, pass, shader, selector, or editor route was found. See [Current Feature Readiness](../../../../../../../Acceptance/CurrentReadiness.md#renderer).

## At A Glance

| Question | Current answer |
| --- | --- |
| Can a user author a look or technical color transform? | No grading parameters, LUT assets, import/cook path, blend stack, pass, shader, or per-view controls exist. |
| Is tone-mapper selection color grading? | No. The current operators are fixed HDR-to-display mappings, not an authored look. |
| Is Linear/sRGB output selection grading? | No. It selects transfer encoding, not color adjustment or gamut conversion. |
| What target was selected? | One scene-referred stage after reconstruction and before target-specific tone mapping, with global controls plus one metadata-bearing 3D LUT generation owned per view. |

The admitted contract is larger than a saturation slider: it must carry authored color-space metadata from asset/import/cook through a deterministic per-view transform and a measurable SDR/HDR result.

## Current Capability

Color grading was not found. Sparkle has no grading pass or shader, per-view grading settings, lift/gamma/gain or slope/offset/power controls, saturation/contrast/hue controls, 1D or 3D grading LUT asset/import/cook path, LUT blend stack, working/display gamut selection, or grading selector/debug product.

The three tone-mapper operators are fixed HDR-to-display mappings. They do not constitute an authored color-grading stack, and output Linear/sRGB encoding is a transfer-function choice rather than grading.

## First-Release Target Contract

The first-release feature is one scene-referred grading stage after reconstruction and before tone mapping. It owns a deterministic global parametric grade plus one optional 3D LUT generation. Unreal's [scene-referred color-grading guidance](https://dev.epicgames.com/documentation/unreal-engine/color-grading-and-the-filmic-tonemapper-in-unreal-engine?lang=en-US) is precedent for keeping artistic correction before display-specific output; it does not define Sparkle's types or prove its result.

The admitted scope must define and implement:

- scene-referred linear input/output, fixed placement before tone mapping, and a separately visible relationship to exposure;
- one ordered global slope/offset/power plus saturation control set and one optional `.cube`-derived 3D LUT generation, with explicit working-space metadata, trilinear interpolation, precision, and identity defaults;
- per-view overrides, requested/active state, defaults, serialization, and editor UX;
- shader/pass inputs and outputs, gamut and alpha behavior, backend requirements, diagnostics, and failure when a LUT or transform is invalid;
- numerical references, identity-transform proof, LUT edge cases, artifact fixtures, cost, and candidate release criteria.

Local volumes, multiple blended LUTs, display-referred legacy LUTs, OpenColorIO, curves, masks, and shot timelines are non-goals for `v0.1.0`. Until the implementation owners exist, documentation and UI report the admitted feature as unavailable and `Blocked`; tone-mapper choice is never a substitute.

## First-Release Acceptance Criteria

- `AC-CGR-01` — zero/default parameters with no LUT are an exact identity at the declared precision, and the graph omits grading work when no non-identity state is active.
- `AC-CGR-02` — the ordered slope/offset/power and saturation transform matches a CPU reference for neutral ramps, primaries, secondaries, gray steps, negative values, and HDR values within a declared tolerance.
- `AC-CGR-03` — one admitted 3D LUT imports and cooks transactionally with working-space, dimension, encoding, interpolation, and content identity; identity and known-cell LUT fixtures match the CPU oracle.
- `AC-CGR-04` — grading is View-owned requested/resolved/active state, persists through the existing settings path, isolates two viewports, and invalidates only its own generation on edit, reload, or LUT replacement.
- `AC-CGR-05` — grading consumes reconstructed scene-referred linear color exactly once before tone mapping; exposure, tone mapping, HDR output, encoding, debug, capture, and UI boundaries remain separately identifiable.
- `AC-CGR-06` — missing, malformed, dimension-mismatched, non-finite, stale, or unsupported LUT/parameter input refuses activation or retains the last explicitly reported good generation without partial publication or silent identity substitution.
- `AC-CGR-07` — D3D12 and Vulkan decoded raw outputs agree with the same numerical oracle; candidate evidence records pass cost, memory, package membership, and clean-machine authored-setting operation.
- `AC-CGR-08` — author controls name their color domain and limits; excluded local-volume, multi-LUT, OCIO, curves, mask, and timeline behavior is neither selectable nor advertised.

## First-Release Failure Modes And Checks

| Failure ID | Controlled setup | Required safe behavior | Check |
| --- | --- | --- | --- |
| `FM-CGR-01` | malformed/non-finite parameters or LUT metadata/data | reject before active publication, identify the field/asset, and preserve a coherent prior/default generation | `CHK-CGR-01`, `CHK-CGR-03` |
| `FM-CGR-02` | LUT load/cook/upload/reload fails or completes after a newer edit | no partial/stale generation becomes active; retirement follows completion identity | `CHK-CGR-03`, `CHK-CGR-05` |
| `FM-CGR-03` | grading is ordered after tone mapping, applied twice, or shares mutable state across views | topology/identity checks fail the candidate; no plausible image counts as a pass | `CHK-CGR-02`, `CHK-CGR-04` |
| `FM-CGR-04` | unsupported/excluded control is restored or advertised | requested state reports unsupported and source/selector/package audit fails the claim | `CHK-CGR-06` |
| `FM-CGR-05` | backend/package output differs or cost/memory exceeds its budget | block the feature; do not hide the route behind silent identity fallback | `CHK-CGR-04`, `CHK-CGR-05` |

| Check | Exercise and oracle | Covers |
| --- | --- | --- |
| `CHK-CGR-01` | CPU-versus-shader parametric transform over identity, ramps, colors, negative/HDR/non-finite inputs | `AC-CGR-01`, `AC-CGR-02`; `FM-CGR-01` |
| `CHK-CGR-02` | graph/resource/domain audit and raw pre-grade/post-grade/pre-tone captures | `AC-CGR-05`; `FM-CGR-03` |
| `CHK-CGR-03` | import/cook/load of identity, known-cell, malformed, wrong-dimension, missing, and corrupt LUT fixtures | `AC-CGR-03`, `AC-CGR-06`; `FM-CGR-01`, `FM-CGR-02` |
| `CHK-CGR-04` | two viewports, settings round trip, live edit/reload, D3D12/Vulkan raw comparison | `AC-CGR-04`, `AC-CGR-07`; `FM-CGR-03`, `FM-CGR-05` |
| `CHK-CGR-05` | exact/over budget, repeated replacement, retirement, package and clean-machine route | `AC-CGR-06`, `AC-CGR-07`; `FM-CGR-02`, `FM-CGR-05` |
| `CHK-CGR-06` | selector/UI/source/package/docs audit for every included and excluded control | `AC-CGR-08`; `FM-CGR-04` |

## Evidence And Source Audit

- `REN-E26` owns the negative source/build/selector/pass/shader/asset/editor/documentation audit for color grading.
- No runtime test is implied by this source-only absence finding.
- Adjacent source routes inspected: [`PostProcessing.cpp`](../../../../../../../../Engine/Renderer/Private/Passes/PostProcessing/PostProcessing.cpp), [`Presentation.cpp`](../../../../../../../../Engine/Renderer/Private/Passes/Presentation/Presentation.cpp), and [`EngineRenderingDisplayTypes.h`](../../../../../../../../Engine/Renderer/Public/Settings/EngineRenderingDisplayTypes.h).

### Current Negative Acceptance

- `AC-CGR-NEG-01` — no selector, per-view setting, asset/import/cook type, LUT, shader/pass, debug product, or editor control advertises color grading.
- `AC-CGR-NEG-02` — tone-mapper and output-encoding choices remain labeled as fixed display mapping/transfer, not an artistic or technical grading stack.

`FM-CGR-NEG-01` occurs when any reachable grading-like vocabulary lacks a real owner/result or when a neighboring transform is mislabeled. `CHK-CGR-NEG-01`/`REN-E26` covers `AC-CGR-NEG-01`, `AC-CGR-NEG-02`, and `FM-CGR-NEG-01` by searching source, build membership, shaders, settings, assets/tools, editor UI, package surfaces, and documentation; any unmatched result fails the negative contract and requires a new current/target dossier before advertisement.
