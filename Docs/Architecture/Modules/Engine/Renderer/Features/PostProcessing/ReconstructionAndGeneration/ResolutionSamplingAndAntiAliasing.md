# Renderer Resolution, Sampling, And Anti-Aliasing

Status: current cross-feature dossier; source-backed, not image-quality, temporal-stability, backend, performance, or release evidence

Verified: 2026-09-07 at committed `master` revision `c28b33bd`

Scope: `REN-RESO-01` through `REN-RESO-07`; output/render extent ownership, viewport/scissor, provider quality resolution, temporal sample policy, attachment sample count, resize/history behavior, and explicit absent anti-aliasing/dynamic-resolution modes

Related owners: [Temporal Sampling and History](../../FrameExecution/TemporalSamplingAndHistory.md) owns jitter/history semantics; [Image Reconstruction and Upscaling](ImageReconstructionAndUpscaling.md) owns Linear/DLSS/RR evaluation; [Presentation and Output](../DisplayPipeline/PresentationAndOutput.md) owns the final target

## Feature Promise And Motivation

For each view, Renderer resolves one output extent and one render extent before graph materialization, constructs view/raster state on that pixel grid, applies one shared temporal sample policy, and produces exactly one output-extent scene color through the selected image-provider route.

This contract prevents four different concepts from being conflated:

- viewport/output size;
- internal render size;
- spatial/temporal reconstruction provider and quality mode;
- raster attachment sample count and anti-aliasing method.

`NativeAA` is a DLSS quality selection, not proof of a generic TAA implementation. An RHI enum admitting sample counts 2/4/8 is not proof that Renderer creates multisampled targets, uses multisample shaders, or resolves MSAA.

## Current Resolution Trace

```text
viewport request extent when valid, otherwise window extent
  -> OutputExtent
  -> choose image pipeline from lighting/reconstruction state
  -> active provider resolves RenderExtent
       no external provider / Linear: RenderExtent = OutputExtent
       DLSS SR or RR: provider queries/stores supported optimal extent
  -> graph topology/resources + RenderView viewport/scissor
  -> Halton jitter normalized by RenderExtent
  -> GBuffer/lighting/history at render extent
  -> exactly one Linear, DLSS SR, or DLSS RR result at OutputExtent
  -> tone/output/product or swapchain presentation
```

Invalid output or provider extents must not produce divided-by-zero jitter or a partially materialized graph. A provider, extent, lighting, shader, or topology change invalidates common/provider history and replaces graph resources by generation/completion rules.

## Current Mode Matrix

| Requested route | Render extent | Sampling/AA meaning | Active/fallback boundary |
| --- | --- | --- | --- |
| Linear | output extent | single-sample raster plus active Halton camera jitter; Linear resolves/copies the current scene result | internal baseline; no separate resolution-scale selector found |
| NVIDIA DLSS SR `NativeAA` | provider-selected extent, expected native/output-sized when provider accepts it | vendor reconstruction quality mode using shared temporal inputs | D3D12/SDK/adapter/interposer gated; initialization failure resolves to Linear |
| NVIDIA DLSS SR Quality/Balanced/Performance/UltraPerformance | provider optimal internal extent | vendor temporal upscaling/reconstruction | requested mode is not active evidence; actual extent/provider must be reported |
| NVIDIA DLSS RR | provider optimal internal extent for eligible ReSTIR route | vendor ray reconstruction owns resolved output | unavailable/unsupported resolves Off and ordinary upscaling owns output |
| RHI sample counts 2/4/8 | vocabulary supported by neutral texture/pipeline validation | potential multisample resource/pipeline contract only | no current Renderer pass/selector/end-to-end resolve route found |
| MSAA jitter helper | source-only 8-sample offset table | temporal sample vocabulary only | active `RenderViewState` hard-codes Halton; helper is not selected |

## Ownership, State, And Invalidation

| Concern | Owner | Required identity |
| --- | --- | --- |
| output extent | viewport request or window fallback in `FramePipeline` | viewport/window generation and exact width/height |
| render extent | image-provider stack and active provider | provider kind/quality/readiness plus output extent |
| graph resources | frame-graph settings/topology generation | render/output extent, format, provider/lighting route |
| viewport/scissor | `RenderViewBuilder`/RHI raster state | current render target extent and view |
| jitter | `RenderViewState` | view identity, render extent, temporal sample index and history validity |
| reconstructed result | RR when active, otherwise upscaler/Linear | one `ResolvedSceneColor` at output extent |
| presentation/product | output/presentation owners | output extent, target/product generation, format and encoding |

A resize or any change that modifies the resolved extents/provider/topology invalidates common temporal and provider history before incompatible consumers. Old graph/provider resources retire after their last real queue use. Per-view state cannot silently borrow another viewport's extent or history.

## Explicit Negative Boundaries

- No Renderer selector or active end-to-end path was found for MSAA, an MSAA resolve, alpha-to-coverage as a product mode, FXAA, SMAA, or a standalone engine TAA pass.
- No frame-time controller, target-frame-budget policy, automatic resolution scaler, min/max percentage, hysteresis, or dynamic-resolution telemetry was found.
- The active temporal sample path is the 16-frame Halton sequence. Jitter alone is not anti-aliasing; the consumer that reconstructs samples and its history behavior must be named.
- Single-sample Renderer attachments may be represented by the default `sampleCount = 1`; frame-graph/pipeline compatibility code accepting other counts is infrastructure, not feature reachability.
- External provider support remains backend/adapter/SDK/driver/interposer gated and does not establish Vulkan parity.

## Acceptance Criteria

- `AC-RESO-01` — valid swapchain/offscreen viewport requests resolve the exact declared output extent; invalid requests use the documented window fallback or reject without a partial graph.
- `AC-RESO-02` — every Linear/DLSS SR/DLSS RR quality cell reports requested provider/mode, active provider/mode, output extent, actual render extent, reason/fallback, and graph/provider generation.
- `AC-RESO-03` — graph resources, viewport, scissor, dispatch dimensions, history resources, provider tags, resolved output, capture metadata, and presentation target agree on their declared render/output extent for unity, odd, one-pixel, resize and supported ratio cases.
- `AC-RESO-04` — Halton jitter uses the actual render extent and shared sign/unit convention; resize/provider/topology changes invalidate all incompatible common and provider history before consumption.
- `AC-RESO-05` — current Renderer raster attachments and pipeline materialization remain sample-count compatible and truthfully report the active single-sample path.
- `AC-RESO-06` — RHI 2/4/8 sample vocabulary and source-only MSAA jitter cannot be selected or advertised as Renderer MSAA until multisampled resources, compatible shaders/pipelines, depth/material semantics, resolve/order, selectors, diagnostics, fallback and evidence exist.
- `AC-RESO-07` — standalone TAA/FXAA/SMAA and dynamic resolution remain explicit not-found cells until a real owner, selector, algorithm/state, quality/cost contract, failure behavior and evidence route exist.

## Controlled Failure Modes And Checks

| Failure ID | Injection or cause | Required safe behavior | Detecting check |
| --- | --- | --- | --- |
| `FM-RESO-01` | zero/invalid/overflowing or rapidly changing extent | graph refuses/rebuilds safely; jitter remains finite/zero as declared; no stale product is published | `CHK-RESO-01`, `CHK-RESO-03` |
| `FM-RESO-02` | provider returns invalid/unsupported extent or requested quality falls back | named active state/fallback; no provider claim and no mixed-extent graph | `CHK-RESO-02` |
| `FM-RESO-03` | pass/resource/dispatch/capture uses output extent where render extent is required, or inverse | dimensional oracle detects mismatch before candidate acceptance | `CHK-RESO-01` |
| `FM-RESO-04` | resize/provider switch retains incompatible history | first changed frame marks history invalid and provider reset | `CHK-RESO-03` |
| `FM-RESO-05` | multisample resource/pipeline mismatch or implicit resolve assumption | graph/pipeline validation rejects; active mode remains single-sample | `CHK-RESO-04` |
| `FM-RESO-06` | dormant sample/AA/dynamic-resolution vocabulary becomes selectable or documented as active | capability/selector audit fails until independently owned and proved | `CHK-RESO-05` |

| Check | Cheapest claim-falsifying exercise | Covers |
| --- | --- | --- |
| `CHK-RESO-01` | graph/resource/dispatch/product dimensional ledger for unity, odd, minimum, resize and supported provider ratios | `AC-RESO-01`, `AC-RESO-03`; `FM-RESO-01`, `FM-RESO-03` |
| `CHK-RESO-02` | provider/backend/quality/readiness matrix recording requested/active states and actual extents | `AC-RESO-02`; `FM-RESO-02` |
| `CHK-RESO-03` | resize and provider/quality/topology churn with exact jitter/history/reset/generation observations | `AC-RESO-03`, `AC-RESO-04`; `FM-RESO-01`, `FM-RESO-04` |
| `CHK-RESO-04` | enumerate all Renderer attachment descriptions and materialized raster pipelines; assert equal active sample counts and exercise an injected mismatch | `AC-RESO-05`, `AC-RESO-06`; `FM-RESO-05` |
| `CHK-RESO-05` | enumerate public settings/CVars/UI, graph routes, passes/shaders, sample-count writes and jitter-pattern selectors | `AC-RESO-06`, `AC-RESO-07`; `FM-RESO-06` |

This contract is **defined but unproved**. A provider returning an extent, a temporally stable screenshot, or RHI sample vocabulary does not prove resolution correctness, anti-aliasing quality, or performance.

Primary evidence destination: `REN-E33` in the [Capability Evidence Plan](../../../../../../../Plans/CapabilityEvidence.md#renderer-capability-to-evidence-map).

## Primary Source Routes

- [`FramePipelineGraph.cpp`](../../../../../../../../Engine/Renderer/Private/Frame/FramePipelineGraph.cpp)
- [`RendererImageProviderStack.cpp`](../../../../../../../../Engine/Renderer/Private/Providers/RendererImageProviderStack.cpp)
- `Engine/Renderer/Private/Upscaling`, `RayReconstruction`, and `Streamline`
- [`RenderViewState.cpp`](../../../../../../../../Engine/Renderer/Private/View/RenderViewState.cpp) and [`TemporalJitterPatterns.cpp`](../../../../../../../../Engine/Renderer/Private/Temporal/TemporalJitterPatterns.cpp)
- `Engine/Renderer/Private/FrameGraph`, `Engine/Renderer/Private/Passes`, and RHI resource/pipeline sample-count contracts
