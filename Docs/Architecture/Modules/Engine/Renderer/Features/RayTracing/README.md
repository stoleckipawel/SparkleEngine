# Renderer Ray Tracing

Status: current feature dossier; source-backed, not native execution, backend parity, visual correctness, performance, or release evidence

Verified: 2026-09-06 against committed `master` revision `d236da11`; `Engine/Renderer` and `Engine/RHI` are unchanged from the earlier `8414b5dc` source audit

Scope: `REN-RT-01` through `REN-RT-06`, the ray portions of `REN-FRONT-02` through `REN-FRONT-04`, and `REN-LGT-02` through `REN-LGT-04`; defines current acceleration-scene, traversal-frontend, shader-table, and effect coverage

Target semantic invariants and their rationale are owned by [Ray-Tracing Execution Architecture](ExecutionArchitecture.md). This dossier states the current feature shape.

## Feature Promise

Sparkle builds a Renderer-owned ray-tracing scene from the same prepared geometry/material/instance identity used by raster rendering. Effects request a semantic operation; the Renderer resolves an inline ray-query or native ray-pipeline frontend from the selected mode and `RhiCapabilities`, then binds the same TLAS/hit/material data and effect output contract.

Ray tracing is capability-gated. Source contains D3D12 and Vulkan acceleration structure, inline query, native pipeline, shader table, and `TraceRays` mechanisms, but this snapshot provides no executable proof that every path runs or matches.

## Acceleration Features

| Feature | Current behavior | Limit |
| --- | --- | --- |
| Static BLAS | cached triangle BLAS derived from resident GPU meshes | one indexed triangle-geometry contract per public description; cache correctness/retirement unproved |
| Deforming BLAS | current skinned/morphed geometry rebuilt for ray use | public Renderer refit/update producer not found; do not claim deforming BLAS refit |
| Classic TLAS | per-view instance build; optional `r.RayTracing.Tlas.Refit` after initial build | transform/identity/update parity needs evidence |
| Partitioned TLAS | provider/capability-gated 3D partition plan | current strategy disables instance updates and partition translation and emits at most one operation |
| Shared identity | classic/PTLAS instances derive from prepared scene and GPU-scene hit records | no independent effect-specific scene/material copy is intended |

The frame graph reserves a persistent `SceneTlas`, declares `RayTracingSceneBuild`, and binds the current native TLAS before graph execution. Traceable instances without matching hit-instance/material records are a fatal contract violation.

## Traversal Coverage

| Effect | Inline ray query | Native pipeline | Active selection |
| --- | --- | --- | --- |
| Ray GBuffer | present | raygen, miss, closest-hit, any-hit present | `r.GBuffer.RayTracingExecution`; Automatic prefers Pipeline then Inline |
| Direct shadow visibility | present | raygen, miss, closest-hit, any-hit present | `r.RayTracing.Shadows.Execution`; independently resolved |
| Reference direct/indirect lighting | present | not found | Inline only |
| ReSTIR indirect temporal/spatial/resolve | present | not found | Inline only |

“Native ray tracing supported” therefore does not mean every ray effect has a native-pipeline frontend. Likewise, an inline-capable device does not establish shader-table/pipeline support.

## Shader-Table Contract

The scene shader-table plan currently has:

- two ray types in fixed contribution order: Surface and ShadowVisibility;
- opaque and alpha-tested triangle hit-group semantics;
- ray-generation, miss, and hit records authored by current effects;
- checked record index `rayContribution + (2 * geometryIndex) + instanceContribution`;
- local record bytes/signatures where required by the RHI contract;
- no current procedural/intersection or callable Renderer programs.

Geometry layout, ray-type layout, hit-group classification, or other SBT semantics change the plan generation. Ordinary material value edits do not redefine logical record indexing. A graph using the scene table rebuilds when that generation changes; old graph/table/pipeline state retires after all last-use queue tokens complete.

## Requested Versus Active Behavior

`Automatic`, `Inline`, and `Pipeline` are requests, not interchangeable labels. Resolution considers acceleration-structure readiness, inline query support, native pipeline support, descriptor indexing/material-table support, registered shader/pipeline completeness, and SBT readiness. A strict unavailable mode must reject graph creation or report an unavailable plan. Automatic may choose the documented alternate but must expose the actual active frontend and reason.

PTLAS selection likewise depends on the provider and advertised operation support. The current narrow PTLAS policy must not be generalized to arbitrary update/move operation support.

## Intent And Tradeoffs

- One semantic effect above thin traversal adapters avoids duplicated GBuffer/shadow meaning. It requires parity evidence at raw outputs, alpha decisions, miss behavior, and motion/deformation edges.
- One shared prepared/GPU scene keeps raster, inline, pipeline, classic, and PTLAS identity coherent. It concentrates lifetime and SBT-index correctness in a few critical owners.
- Stable checked SBT planning prevents material-value churn from rebuilding logical record layout. It introduces an explicit generation dependency when geometry/hit-group meaning changes.
- Rebuilding deforming BLAS is a clear current policy but can be expensive; performance must be measured before introducing update/refit complexity.
- Vendor PTLAS is isolated behind RHI provider capabilities while Renderer owns partition policy. This preserves the boundary but creates an intentionally asymmetric optional path.

## Failure, Diagnostics, And Evidence

- Missing TLAS or mismatched hit/material counts fail loudly at the frame-graph binding boundary.
- Unsupported strict execution modes must not silently fall back; Automatic fallback must be observable.
- SBT out-of-range/incorrect contribution, alpha-tested any-hit, repeated geometry/material identities, move/delete/reload, deformed geometry, and retirement need controlled captures and native validation.
- Primary checks: `RHI-E08` through `RHI-E11`, `RHI-E15`, `REN-E04`, `REN-E08`, `REN-E11`, `REN-E12`, and `REN-E19`.

## Acceptance Criteria

- `AC-RT-01` — static, instanced, alpha-tested, double-sided, skinned, morphed, moved, removed, and reloaded triangle instances preserve one scene/hit/material identity through BLAS, classic/PTLAS, inline, pipeline, and retirement where each cell is supported.
- `AC-RT-02` — static BLAS reuse occurs only while geometry identity is unchanged; deforming geometry rebuild is reported as rebuild and produces current positions without being mislabeled refit/update.
- `AC-RT-03` — classic TLAS initial build and enabled refit/update preserve transforms, instance IDs, masks, and shader-table contribution; unsupported update conditions rebuild or reject explicitly.
- `AC-RT-04` — PTLAS activates only when the provider supports the exact requested operations; the current one-operation/no-instance-update/no-translation limit is enforced and visible.
- `AC-RT-05` — SBT indices for both ray types and every opaque/alpha-tested geometry fall within the planned record count, point to the intended hit group/local data, and invalidate only on layout-semantic changes.
- `AC-RT-06` — each effect resolves and reports requested versus active execution independently; strict unavailable frontends reject and Automatic selects only a fully ready documented alternate.
- `AC-RT-07` — Inline and Pipeline GBuffer/direct-shadow adapters agree on semantic outputs for miss, opaque, alpha, repeated geometry/material, and deformation fixtures; effects without a Pipeline adapter remain explicitly Inline-only.
- `AC-RT-08` — missing TLAS, hit/material records, descriptor capacity, program, pipeline, SBT, or backend capability fails before trace dispatch and names the incomplete contract.
- `AC-RT-09` — graph/SBT/pipeline/BLAS/TLAS generations remain alive through last-use queue tokens and are reclaimed after completion without stale binding during churn or shutdown.
- `AC-RT-10` — applicable D3D12/Vulkan cells pass native validation and predeclared raw-output parity; unsupported cells are independently Excluded, not generalized from another backend/frontend.

## Controlled Failure Modes

| ID | Injection or cause | Required safe behavior | Affected criteria |
| --- | --- | --- | --- |
| `FM-RT-01` | strict frontend missing one capability/program/SBT dependency | plan/graph resolution rejects and reports the missing dependency | `AC-RT-06`, `AC-RT-08` |
| `FM-RT-02` | SBT contribution/index/count mismatch or stale plan generation | validation rejects before table publication/dispatch; prior valid generation remains isolated | `AC-RT-05`, `AC-RT-08`, `AC-RT-09` |
| `FM-RT-03` | traceable instance lacks matching hit/material/texture record | GPU-scene/graph binding fails; no plausible hit shading is published | `AC-RT-01`, `AC-RT-08` |
| `FM-RT-04` | request unsupported PTLAS update/translation or overflow operation plan | PTLAS remains inactive/rejected with exact operation reason; no silent classic/PTLAS claim swap | `AC-RT-04`, `AC-RT-06` |
| `FM-RT-05` | geometry/material/scene changes while old work is in flight | new generation binds only new state; old resources retire after every queue token | `AC-RT-01`, `AC-RT-09` |
| `FM-RT-06` | frontend/backend differs at alpha, miss, bias, or deformation edge | parity oracle fails and holds only the affected cell with artifacts retained | `AC-RT-07`, `AC-RT-10` |

## Checks And Completion

| Check | Exercise and oracle | Covers |
| --- | --- | --- |
| `CHK-RT-01` | deterministic scene-identity/BLAS/TLAS matrix over static/deforming/add/move/remove/reload, classic refit on/off, and PTLAS capability/operation limits | `AC-RT-01`–`AC-RT-04`; `FM-RT-03`, `FM-RT-04` |
| `CHK-RT-02` | enumerate planned SBT records and recompute every logical index; corrupt contribution/count/generation and require pre-dispatch rejection | `AC-RT-05`, `AC-RT-08`; `FM-RT-02` |
| `CHK-RT-03` | effect/frontend/capability matrix for GBuffer and shadows plus explicit Inline-only reference/ReSTIR-indirect cells; compare raw outputs and active-state diagnostics | `AC-RT-06`–`AC-RT-08`; `FM-RT-01`, `FM-RT-06` |
| `CHK-RT-04` | churn geometry, shader/SBT, graph, resize, reload, and shutdown while frames are in flight; inspect bindings, queue tokens, retained generations, and reclamation | `AC-RT-09`; `FM-RT-05` |
| `CHK-RT-05` | focused D3D12/Vulkan execution with native validation, raw GBuffer/visibility comparison, and per-cell capability report | `AC-RT-07`, `AC-RT-10`; `FM-RT-06` |

This current contract is **defined but unproved**. The target [Execution Architecture](ExecutionArchitecture.md) does not upgrade any cell: only candidate-bound runtime, raw-output, lifetime, and native-validation artifacts can do so.

## Primary Source Routes

- [`RenderRayTracingScene.cpp`](../../../../../../../Engine/Renderer/Private/Scene/RayTracing/RenderRayTracingScene.cpp)
- [`RayTracingScene.cpp`](../../../../../../../Engine/Renderer/Private/Passes/RayTracing/RayTracingScene.cpp)
- [`RayTracingBlasCache.cpp`](../../../../../../../Engine/Renderer/Private/RayTracing/Acceleration/RayTracingBlasCache.cpp), [`RayTracingClassicTlasStrategy.cpp`](../../../../../../../Engine/Renderer/Private/RayTracing/Acceleration/RayTracingClassicTlasStrategy.cpp), and the adjacent PTLAS strategy owners
- [`RayTracingShaderTablePlan.cpp`](../../../../../../../Engine/Renderer/Private/Scene/RayTracing/RayTracingShaderTablePlan.cpp)
- [`RayTracingGBuffer.cpp`](../../../../../../../Engine/Renderer/Private/Passes/GBuffer/RayTracingGBuffer.cpp)
- [`DirectShadowSignal.cpp`](../../../../../../../Engine/Renderer/Private/Passes/Lighting/Shadows/DirectShadowSignal.cpp)
- [RHI Ray Tracing](../../../RHI/Features/PipelineAndExecution/RayTracing.md) for the mechanism and local proof contract, [RHI Capability Inventory](../../../RHI/CapabilityInventory.md#ray-tracing-coverage) for exact backend rows, and [Shader Program Catalog](../ShaderRuntime/ShaderProgramCatalog.md) for current ray stages.
