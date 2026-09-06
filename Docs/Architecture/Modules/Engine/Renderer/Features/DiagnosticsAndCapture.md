# Renderer Diagnostics, Products, and Capture

Status: current feature dossier; source-backed, not proof that diagnostics are correct, complete, low-overhead, or release-safe

Verified: 2026-09-06 against committed `master` revision `8414b5dc`

Scope: `REN-DIAG-01` through `REN-DIAG-08`; defines Renderer observability, viewport products, asynchronous capture, previews, and shader-generation reload from the frame owner's perspective

## Feature Promise

Renderer exposes bounded observations of the frame it actually prepared/submitted and publishes named render products that editor and capture consumers can identify. Diagnostics must describe active state and failure; they are not allowed to become a second implementation authority or imply that a frame was correct merely because it completed.

## Current Diagnostic And Product Features

| Feature | Producer -> consumer | Current result and limit |
| --- | --- | --- |
| Frame/pass diagnostics | `FramePipeline`/frame graph/RHI scopes -> renderer read state/editor | frame execution, pass execution, marker, optional timestamp, and graph-contract observations |
| GPU markers | `r.Diagnostics.MarkerVerbosity` Off/FramePass/Detailed -> backend events | profiler-visible frame/pass names; detailed mode may enable draw/dispatch diagnostics |
| GPU timing | `r.Diagnostics.GpuTiming` -> timestamp queries -> later frame resolution | opt-in internal timings; availability and observer cost depend on backend |
| Mesh diagnostics | GPU mesh cache -> snapshot | cache/residency/detail information exposed through Renderer facade |
| Texture diagnostics | texture cache -> snapshot/editor texture registration | cache/residency rows and UI-usable texture handles |
| Memory diagnostics | Renderer memory monitor + cache/RHI budgets -> snapshot | combined renderer/resource budget view ticked at frame begin |
| Viewport products | frame graph -> `ViewportRenderProductPublication` | final color, scene depth, normals, extents and generation identity |
| Async capture | public request -> coordinator -> `ViewportCaptureService` -> RHI readback -> completion queue | requested viewport/intermediate product read back without blocking submission |
| Mesh preview | Renderer preview product/handle route -> editor | editor-consumable mesh preview; fidelity/usability unproved |
| Shader reload | recook coordinator -> validated runtime cache -> graph invalidation/retirement | whole shader generation swaps only after materialization, old state retires by queue completion |

## Capture Lifecycle

```text
BeginViewportCapture(request, id)
  -> resolve current named product; check expected frame identity
  -> begin RHI asynchronous texture readback
  -> PollFrameServices on later frames
  -> move completed readback to coordinator read state
  -> caller TryTakeViewportCapture moves result out
```

The capture service permits at most three pending requests and retains at most three completed captures, dropping the oldest completed result beyond that bound. “No result yet” means pending or absent, not success. The current result records frame, scene, and provider generations plus artifact/failure and readback dimensions/format; it does not carry shader or graph-topology generation. Requested-versus-resolved product and color/encoding provenance therefore remain documentation/evidence gaps rather than implied metadata.

## Where Observation Occurs In A Frame

- At frame start, completed capture, retirement, provider/shader retirement, and residency work is polled.
- `BeginFrame` resolves prior GPU timings and ticks memory state for the new frame ID.
- Graph execution emits frame/pass scopes, compiled-barrier/alias markers, and optional detailed command markers.
- Product publication identifies resources before graph execution so later capture requests can resolve the owning graph generation.
- After submission, queue tokens become the authority for upload, graph, provider, and shader retirement.

This sequence makes diagnostics causally attachable to a frame, but correlation with PIX, RenderDoc, Nsight, RGP, DRED, or Vulkan validation remains an evidence task.

## Failure And Trust Boundaries

- Invalid/unavailable product or unsupported readback format must report failure; an empty image or stale prior product is not success.
- Diagnostic buffers/queues must expose truncation or drops. A bounded container that silently loses the newest relevant failure is not trustworthy.
- GPU timestamp values require valid queue frequency/order and must not be compared across unsupported clock domains.
- Enabling detailed markers/timing/capture changes observer cost; performance evidence must record diagnostic configuration.
- Development diagnostics, private paths, shader debug artifacts, or editor handles must not leak into the Shipping runtime package.
- A responsive process, submitted command list, nonempty capture, or plausible metric is not proof of visual/numerical correctness.

## Intent And Tradeoffs

- Observability lives beside the owner that can attach correct identities, while native validation/crash data stays in RHI. This avoids Renderer guessing backend state.
- Capture is asynchronous to preserve frame progress. The tradeoff is explicit pending/completion/drop and lifetime management.
- Whole-generation shader reload prevents partial program/pipeline mixes. It can retain old generations longer when GPU completion stalls.
- Bounded snapshots protect runtime memory but require visible truncation/drop semantics and support-oriented prioritization.

Primary evidence: `REN-E19`, `REN-E21`, `RHI-E13`, `RHI-E14`, `ED-E03`, and the external-capture workflow in `WF-011`/`WF-017`.

## Primary Source Routes

- [`FramePipeline.cpp`](../../../../../../Engine/Renderer/Private/Frame/FramePipeline.cpp) and [`FramePipelineDiagnostics.cpp`](../../../../../../Engine/Renderer/Private/Frame/FramePipelineDiagnostics.cpp)
- [`FrameExecutionDiagnostics.cpp`](../../../../../../Engine/Renderer/Private/Diagnostics/FrameExecutionDiagnostics.cpp) and [`PassExecutionDiagnostics.cpp`](../../../../../../Engine/Renderer/Private/Diagnostics/PassExecutionDiagnostics.cpp)
- [`RendererMemoryMonitor.cpp`](../../../../../../Engine/Renderer/Private/Diagnostics/RendererMemoryMonitor.cpp)
- [`ViewportCaptureService.cpp`](../../../../../../Engine/Renderer/Private/Viewport/ViewportCaptureService.cpp)
- [`ViewportRenderProductPublication.cpp`](../../../../../../Engine/Renderer/Private/Viewport/ViewportRenderProductPublication.cpp)
- [`RenderPassRuntimeCache.cpp`](../../../../../../Engine/Renderer/Private/Pipeline/RenderPassRuntimeCache.cpp)
- [Performance Diagnostics](../../../../CrossModule/PerformanceDiagnostics/README.md) for cross-module/external-tool architecture and [Validation and Evidence](../../../../../Engineering/Verification/ValidationAndEvidence.md) for proof grades.
