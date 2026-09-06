# Renderer Tone Mapping

Status: current feature dossier; source-backed, not numerical, colorimetric, display, backend, or release evidence

Verified: 2026-09-06 against source revision `d236da11`; `Engine/Renderer` is unchanged from the earlier `8414b5dc` source audit

Scope: `REN-POST-07`; exposure-weighted HDR scene-referred color to display-linear color through one selected tone-mapping operator

Parent family: [Post Processing](README.md)

## Feature Promise

Sparkle multiplies output-extent `ResolvedSceneColor` by the current 1x1 exposure value and applies exactly one selected operator into `ToneMappedSceneColor`. The result remains display-linear; [Presentation and Output](PresentationAndOutput.md) owns later transfer encoding and publication.

| Selector | Default | Reachable choices | Current boundary |
| --- | --- | --- | --- |
| `r.ToneMapper` / resolved per-view `EngineToneMapper` | ACES approximation | Reinhard, ACES approximation, ACES fitted filmic | no public None, raw bypass, or custom operator |

The shader clamps alpha with `saturate` while mapping RGB. Current source does not establish reference-ACES conformance, working/display gamut transforms, scene-white/nit semantics, artistic grading, or exact curve accuracy.

## Ownership And Frame Placement

- `RenderView` owns the resolved per-view tone-mapper intent; `ExecuteRenderFrameGraph` updates the graph uniform each frame.
- Tone mapping consumes the resolved scene color after reconstruction/upscaling and optional debug replacement.
- Exposure is multiplied in the tone-mapping shader, but [Exposure](Exposure.md) owns metering and temporal adaptation.
- Tone mapping is not color grading: no grading controls, grading stack, or LUT is applied before or inside the current operator.
- Output encoding is a separate pass and remains the only owner of Linear/sRGB transfer selection.

## Failure, Tradeoffs, And Evidence

- Unknown enum values are fatal settings defects rather than silent substitutions.
- One common operator boundary keeps lighting and provider routes from inventing their own display transforms, but the lack of a None/bypass mode prevents exact raw debug presentation.
- `REN-E17` owns known HDR ramps, finite/extreme input behavior, all three operators, alpha behavior, and interaction with exposure/output encoding.
- Stable numerical and color-domain obligations are defined below; execution remains pending.

## Acceptance Criteria

- `AC-TMO-01` — Reinhard, ACES approximation, and ACES fitted filmic match a pinned CPU/reference implementation over black, gray, primary, negative, HDR ramp, and extreme finite inputs within declared precision.
- `AC-TMO-02` — exposure is multiplied exactly once before the selected curve; changing exposure produces the reference result and no provider/debug/presentation stage duplicates it.
- `AC-TMO-03` — output is `R16G16B16A16_Float` display-linear RGB at output extent; output encoding and color grading are not performed in this pass.
- `AC-TMO-04` — alpha follows the documented saturated-input policy independently of RGB, including negative, unit, greater-than-one, NaN, and Inf cases.
- `AC-TMO-05` — unknown selector values fail settings/graph resolution; all three valid operators are reported as requested/active per view with no silent substitution.
- `AC-TMO-06` — D3D12 and Vulkan decoded outputs agree within the predeclared numeric tolerance for every operator/exposure case.

## Controlled Failure Modes And Checks

| Failure ID | Injection and safe state | Detecting check |
| --- | --- | --- |
| `FM-TMO-01` | invalid operator enum | resolve rejects before dispatch and identifies the invalid value | `CHK-TMO-02` |
| `FM-TMO-02` | NaN/Inf/extreme RGB or alpha | output follows the predeclared finite/non-finite policy; no unclassified value is accepted | `CHK-TMO-01` |
| `FM-TMO-03` | exposure or output transfer is accidentally applied twice | reference chain comparison fails at the first incorrect stage | `CHK-TMO-01`, `CHK-TMO-03` |
| `FM-TMO-04` | backend shader/compiler result exceeds tolerance | affected backend/operator cell remains Pending/Blocked with raw artifacts | `CHK-TMO-03` |

| Check | Exercise and oracle | Covers |
| --- | --- | --- |
| `CHK-TMO-01` | deterministic shader readback against pinned CPU curves over operator × exposure × RGB/alpha vectors | `AC-TMO-01`–`AC-TMO-04`; `FM-TMO-02`, `FM-TMO-03` |
| `CHK-TMO-02` | selector/per-view test over every valid operator plus invalid value and dual-viewport choices | `AC-TMO-05`; `FM-TMO-01` |
| `CHK-TMO-03` | paired-backend raw display-linear readback followed by separate encoding verification | `AC-TMO-02`, `AC-TMO-03`, `AC-TMO-06`; `FM-TMO-03`, `FM-TMO-04` |

This contract is **defined but unproved**. It does not claim reference ACES color management; evidence must name the exact approximation implemented and compare the unencoded display-linear product.

## Primary Source Routes

- [`ToneMapping.hlsl`](../../../../../../../Engine/Assets/Shaders/Passes/Presentation/ToneMapping.hlsl)
- [`ToneMapping.hlsli`](../../../../../../../Engine/Assets/Shaders/Display/ToneMapping.hlsli)
- [`ToneMappingSettings.cpp`](../../../../../../../Engine/Renderer/Private/Passes/Presentation/ToneMappingSettings.cpp)
- [`Presentation.cpp`](../../../../../../../Engine/Renderer/Private/Passes/Presentation/Presentation.cpp)
