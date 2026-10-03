# Stage 8 Source Evidence

**Status:** source implementation complete; executable parity evidence deferred

**Source input:** `1bfccd778e49fc425b344c490a30c2adf9513e33` plus the scoped post-integration cleanup working tree

This record covers the source-shape result of Stage 8. It does not claim a successful build, shader cook, GPU execution, native-validation run, numeric comparison, performance result, or accepted D3D12/Vulkan parity result.

## `ARCH-RPT-8` Ownership Ledger

| Concern | Owner and implemented shape |
| --- | --- |
| Automatic frontend resolution | `RenderRayTracingScene` resolves the one engine policy once from its immutable capability report: shared requirements first, Pipeline when available, Inline otherwise, and `None` when neither complete route exists. `AddRayTracingMaterialPass` consumes that scene-owned result for GBuffer, direct-shadow, and Reference pass scheduling; no effect owns an execution-mode CVar, request, resolver, plan wrapper, switch, or fallback order. |
| Reference policy | `ReferencePathTracerSession` retains the canonical ray-tracing scene reference required by pass construction and consumes only its resolved frontend for session identity and composition. `AddReferencePathTracerPasses` is the stateless graph-composition entry. Sample identity, estimator consequence, accumulation, progress, and reset policy remain inside the feature capsule. Traversal selection is not a view-mode or Reference-specific setting. |
| Semantic trace boundary | `RayTracingSceneTrace.hlsli` declares the one feature-facing, material-alpha-aware `TraceSceneRay` signature and `RayTracingTraceResult` remains the shared event result. Thin Inline and Pipeline headers implement that signature; only those adapters contain `TraceRayInline` or `TraceRay`. Callers may use standard ray flags such as `RAY_FLAG_FORCE_OPAQUE` without selecting a frontend-specific function. |
| Pipeline payload and hit mechanics | `RayTracingMaterialPayload.hlsli`, the paired Renderer `ShaderData/RayTracingMaterialPayload.h` ABI layout, `RayTracingMaterialPipeline.hlsl`, and `RayTracingMaterialPipelineShaders.h` own the reusable full-hit payload plus miss, closest-hit, and alpha any-hit mechanics. C++ metadata derives payload and built-in triangle-attribute byte sizes from standard-layout ABI types rather than copied literals. `AddRayTracingMaterialPass` is the one dual-frontend scheduling and native material-composition primitive used by ray-traced GBuffer, direct shadows, and Reference. |
| Shared pass binding | `BindSceneShaderParameters` binds only the canonical Scene/View/TLAS/material/light fields declared by each typed pass. Feature bodies retain their outputs, histories, dispatch shape, and estimator policy instead of copying common resource/setup blocks or accepting broad dependency bags. |
| Pipeline/SBT mechanism | Existing `RayTracingPipelineComposition`, `RayTracingPipelineRuntime`, `RayTracingShaderTablePlan`, frame-graph `TraceRays`, and D3D12/Vulkan RHI pipeline/table/dispatch paths are reused unchanged. No Reference type or policy enters RHI and no second submission route exists. |
| Graph topology | The original frame retains one Lit-versus-Reference branch. Reference selects one Inline compute or native ray-generation transport pass, followed by the same accumulator display, exposure, viewport product, post-processing, frame-graph execution, RHI submission, and presentation shell. The frontend is immutable for the ray-tracing scene; only a pipeline-relevant shader-table generation participates in graph reconstruction identity. |
| Capability refusal | An unavailable automatic resolution produces `None`; the session reports `UnsupportedCapability`, publishes no active route/backend, allocates no session textures, and cannot bind or execute the graph. |

No `RayTracingShaderTablePlan` or RHI contract extension was needed. The existing scene layout already supplies the surface records used by the common full-hit trace payload, so another record vocabulary would have duplicated current mechanism without a consumer.

## `CORE-RPT-8` Semantic Ledger

| Invariant | Source correspondence |
| --- | --- |
| One estimator | `ReferencePathTracerKernel.hlsli` owns camera/sample construction, the call to `TraceSurfaceTransport`, and accumulation. Both entry shaders call `TraceAndAccumulate`; neither contains a second estimator. |
| Traversal-only adapters | `ReferencePathTracerInline.hlsl` includes the RayQuery implementation and dispatches the shared kernel. `ReferencePathTracerPipeline.hlsl` includes the native `TraceRay` implementation and dispatches the same kernel. |
| Shared visibility semantics | `PathVisibility.hlsli` consumes the same material-alpha trace contract as continuation rays. Instance/primitive identity used for sampled-emitter visibility therefore has the same result vocabulary on both frontends. |
| Same material and alpha path | Both frontends reconstruct `RayTracingHitSurfaceData` through the existing shared material/hit owner. The native any-hit shader calls the same alpha evaluator used by Inline candidate acceptance. |
| Same sample prefix and accumulation | Sampler dimensions, random words, camera rays, path loop, BSDF/light/MIS/roulette code, invalid-radiance consequence, row scheduling, committed prefix, and mean/M2 accumulation are unchanged and occur after the frontend boundary. |
| Route-bearing identity | The resolved frontend is part of session identity. A capability-driven route change invalidates before a different frontend can contribute to the retained prefix. Backend identity remains separate. |

## `FRAME-RPT-8` Route Trace

```text
ViewportRenderRequest::ViewMode == ReferencePathTracer
  -> FramePipeline::BuildRenderFrameGraph
  -> AddRayTracingMaterialPass consumes RenderRayTracingScene frontend
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
- shared material pipeline shaders consumed by GBuffer, direct shadows, and Reference;
- exactly one `TraceSceneRay` API, one raw `TraceRay` adapter, and one `TraceRayInline` adapter;
- zero per-effect ray-tracing execution CVars, settings, plan wrappers, or selector aliases;
- no backend conditional in Reference HLSL or estimator code;
- no estimator, random-dimension, material/light, invalid-result, accumulation, or camera-reset conditional on frontend, backend, or `RenderViewKind`;
- no wavefront, SER, vendor path, fallback, diagnostics framework, readback, or export addition.

The source-only checks below were run against the final scoped diff:

| Static check | Result |
| --- | --- |
| changed C/C++ `clang-format` 22.1.3 dry run with `--Werror --style=file` | PASS |
| `cmake -DSPARKLE_REPO_ROOT=... -P CMake/ArchitectureBoundaryCheck.cmake` | PASS; no new violations |
| one-estimator/two-adapter, shared-consumer, forbidden feature/backend fork, stale-symbol, and RHI-policy probes | PASS; one raw `TraceRay`, one `TraceRayInline`, and eight `TraceSceneRay` declarations/calls |
| all seven changed shader include paths, all registration virtual paths, and exact 32-program catalog count | PASS |
| 29 changed-document local targets, strict UTF-8, trailing whitespace, and `git diff --check` | PASS |

The following owner-operated evidence is deliberately deferred and must not be reported as passed:

- focused Renderer/Editor builds and shader cooking;
- D3D12 Inline, D3D12 Pipeline, Vulkan Inline, and Vulkan Pipeline GPU runs;
- native D3D12 and Vulkan validation output inspection;
- identical analytic jobs, sample-prefix/raw comparisons, and statistical tolerance evaluation;
- Editor `Scene` and Game `Game` live camera reset/refinement/completion checks;
- compiler/settings identity capture, bounded-progress timing, camera-response timing, and TDR-budget measurement.

Accordingly, Stage 8 is **IMPLEMENTED / VALIDATION DEFERRED** at source level. Implementation may continue to Stage 9 source work under the plan's deferred-validation policy, but `AC-RPT-17`, backend/frontend parity, performance safety, and Reference authority remain unproved.
