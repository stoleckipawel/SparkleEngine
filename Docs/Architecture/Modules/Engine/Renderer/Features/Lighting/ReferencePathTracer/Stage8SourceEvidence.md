# Stage 8 Source Evidence

**Status:** source implementation complete; executable parity evidence deferred

**Source input:** `e6b27e679369bc153a32d9d434b4e0a624d7c8f9` plus the scoped Stage-8 working tree

This record covers the source-shape result of Stage 8. It does not claim a successful build, shader cook, GPU execution, native-validation run, numeric comparison, performance result, or accepted D3D12/Vulkan parity result.

## `ARCH-RPT-8` Ownership Ledger

| Concern | Owner and implemented shape |
| --- | --- |
| Requested/active frontend resolution | Shared `RayTracingExecutionPlan` resolves strict `Inline`, strict `Pipeline`, and ordered automatic policy from the existing capability report. GBuffer, direct-shadow, and path-tracing consumers use that one resolver; Reference automatic policy is the frozen Inline-then-Pipeline order. |
| Reference policy | `ReferencePathTracer` reads `r.PathTracing.Execution` at its consumption boundary, supplies the resolved plan to its session and pass composition, and keeps sample identity, estimator consequence, accumulation, progress, and reset policy inside the feature capsule. The CVar selects traversal mechanism, not the view mode. |
| Semantic trace boundary | `RayTracingMaterialTrace.hlsli` owns one backend-neutral material-alpha trace signature and `RayTracingTraceResult` remains the shared event result. Inline and Pipeline headers implement that signature without changing estimator inputs or outputs. |
| Pipeline payload and hit mechanics | `RayTracingMaterialPayload.hlsli`, the paired Renderer `ShaderData/RayTracingMaterialPayload.h` ABI layout, `RayTracingMaterialPipeline.hlsl`, and `RayTracingMaterialPipelineShaders.h` own the reusable full-hit payload plus miss, closest-hit, and alpha any-hit mechanics. C++ metadata derives payload and built-in triangle-attribute byte sizes from standard-layout ABI types rather than copied literals. The ray-traced GBuffer and Reference pipeline compositions consume the same shader types and one shared material metadata constant. |
| Pipeline/SBT mechanism | Existing `RayTracingPipelineComposition`, `RayTracingPipelineRuntime`, `RayTracingShaderTablePlan`, frame-graph `TraceRays`, and D3D12/Vulkan RHI pipeline/table/dispatch paths are reused unchanged. No Reference type or policy enters RHI and no second submission route exists. |
| Graph topology | The original frame retains one Lit-versus-Reference branch. Reference selects one Inline compute or native ray-generation transport pass, followed by the same accumulator display, exposure, viewport product, post-processing, frame-graph execution, RHI submission, and presentation shell. The resolved frontend participates in graph reconstruction identity only while Reference is selected. |
| Capability refusal | A strict unsupported frontend resolves to `None`; the session reports `UnsupportedCapability`, publishes no active route/backend, allocates no session textures, and cannot bind or execute the graph. There is no frontend fallback for a strict request. |

No `RayTracingShaderTablePlan` or RHI contract extension was needed. The existing scene layout already supplies the surface records used by the common full-hit trace payload, so another record vocabulary would have duplicated current mechanism without a consumer.

## `CORE-RPT-8` Semantic Ledger

| Invariant | Source correspondence |
| --- | --- |
| One estimator | `ReferencePathTracerKernel.hlsli` owns camera/sample construction, the call to `TraceSurfaceTransport`, and accumulation. Both entry shaders call `TraceAndAccumulate`; neither contains a second estimator. |
| Traversal-only adapters | `ReferencePathTracerInline.hlsl` includes the RayQuery implementation and dispatches the shared kernel. `ReferencePathTracerPipeline.hlsl` includes the native `TraceRay` implementation and dispatches the same kernel. |
| Shared visibility semantics | `PathVisibility.hlsli` consumes the same material-alpha trace contract as continuation rays. Instance/primitive identity used for sampled-emitter visibility therefore has the same result vocabulary on both frontends. |
| Same material and alpha path | Both frontends reconstruct `RayTracingHitSurfaceData` through the existing shared material/hit owner. The native any-hit shader calls the same alpha evaluator used by Inline candidate acceptance. |
| Same sample prefix and accumulation | Sampler dimensions, random words, camera rays, path loop, BSDF/light/MIS/roulette code, invalid-radiance consequence, row scheduling, committed prefix, and mean/M2 accumulation are unchanged and occur after the frontend boundary. |
| Route-bearing identity | Requested and active frontend are part of session identity. A route change invalidates before a different frontend can contribute to the retained prefix. Backend identity remains separate. |

## `FRAME-RPT-8` Route Trace

```text
ViewportRenderRequest::ViewMode == ReferencePathTracer
  -> FramePipeline::BuildRenderFrameGraph
  -> ReferencePathTracer::ResolveExecutionPlan
  -> exactly one of:
       ReferencePathTracerInlineCS
       ReferencePathTracerRGS + shared material hit shaders + scene SBT
  -> shared ReferencePathTracer::TraceAndAccumulate shader kernel
  -> feature-local committed display
  -> shared exposure / native-resolution viewport / post-processing
  -> existing frame-graph execution and D3D12 or Vulkan RHI submission
```

Editor `Scene` and Game/runtime `Game` continue to submit the same ordinary mode and camera/View contract. Neither host selects a shader, payload, estimator, backend lowering, or submission path.

## Source Checks And Deferred Evidence

The scoped source audit requires:

- zero Reference-prefixed generic ray, payload, hit-reconstruction, material, alpha, or RHI types;
- exactly one Reference transport kernel and two entry adapters;
- shared material pipeline shaders consumed by both GBuffer and Reference;
- no backend conditional in Reference HLSL or estimator code;
- no estimator, random-dimension, material/light, invalid-result, accumulation, or camera-reset conditional on frontend, backend, or `RenderViewKind`;
- no wavefront, SER, vendor path, fallback, diagnostics framework, readback, or export addition.

The source-only checks below were run against the final scoped diff:

| Static check | Result |
| --- | --- |
| changed C/C++ `clang-format` 22.1.3 dry run with `--Werror --style=file` | PASS |
| `cmake -DSPARKLE_REPO_ROOT=... -P CMake/ArchitectureBoundaryCheck.cmake` | PASS; no new violations |
| one-estimator/two-adapter, shared-consumer, forbidden feature/backend fork, stale-symbol, and RHI-policy probes | PASS |
| 13 changed/new shader include paths, registration virtual paths, and dedicated HLSL attribute lines | PASS |
| changed-document local targets, strict UTF-8, trailing whitespace, and `git diff --check` | PASS |

The following owner-operated evidence is deliberately deferred and must not be reported as passed:

- focused Renderer/Editor builds and shader cooking;
- D3D12 Inline, D3D12 Pipeline, Vulkan Inline, and Vulkan Pipeline GPU runs;
- native D3D12 and Vulkan validation output inspection;
- identical analytic jobs, sample-prefix/raw comparisons, and statistical tolerance evaluation;
- Editor `Scene` and Game `Game` live camera reset/refinement/completion checks;
- compiler/settings identity capture, bounded-progress timing, camera-response timing, and TDR-budget measurement.

Accordingly, Stage 8 is **IMPLEMENTED / VALIDATION DEFERRED** at source level. Implementation may continue to Stage 9 source work under the plan's deferred-validation policy, but `AC-RPT-17`, backend/frontend parity, performance safety, and Reference authority remain unproved.
