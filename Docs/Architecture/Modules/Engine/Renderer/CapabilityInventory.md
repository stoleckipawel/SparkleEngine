# Renderer Capability Inventory

Status: capability snapshot; not release approval, visual validation, or performance evidence

Snapshot: 2026-09-06 through committed `master` revision `c28b33bd`; current `Engine/Renderer` public surface, CMake membership, Scene/View/Frame path, frame graph, passes, ray-tracing paths, providers, and shader registrations inspected; executable source is unchanged from the earlier `8414b5dc` audit; evidence `S` only

Scope: render-side ownership, frame production, graph scheduling, pipeline materialization and typed binding, scene/view and temporal preparation, residency, geometry/material/lighting algorithms, ray execution, post processing, providers, UI, latency coordination, settings lifecycle, debug products, diagnostics, and known coverage gaps

Owner: `Engine/Renderer`

Supporting maps: [Whole Repository Architecture Map](../../../WholeRepositoryMap.md), [Renderer and RHI Architecture Boundary](../../../Decisions/RendererRhiBoundary.md), and [Ray-Tracing Execution Architecture](Features/RayTracing/ExecutionArchitecture.md)

Evidence plan and release disposition: [Capability Evidence Plan](../../../../Plans/CapabilityEvidence.md) and [First Release Acceptance Contract](../../../../Acceptance/FirstRelease.md)

Traceability: capability rows use durable `REN-<family>-NN` identities; their primary proof destinations are listed in the [Renderer capability-to-evidence map](../../../../Plans/CapabilityEvidence.md#renderer-capability-to-evidence-map).

Deeper routes: [Rendering a Sparkle Frame](RenderingASparkleFrame.md), [Renderer feature dossiers](Features/README.md), [pipeline materialization and typed binding](Features/ShaderRuntime/PipelineMaterializationAndTypedBinding.md), [scene and view preparation](Features/SceneAndViewPreparation/README.md), [mesh and texture residency](Features/GeometryAndResources/MeshAndTextureResidency.md), [temporal sampling and history](Features/FrameExecution/TemporalSamplingAndHistory.md), [latency coordination](Features/FrameExecution/LatencyCoordination.md), [settings lifecycle](Features/RuntimeConfiguration/SettingsStateAndPersistence.md), [cross-system graphics coverage](../../../CrossModule/GraphicsCoverageMatrix.md), [producer-to-consumer execution traces](../../../CrossModule/FeatureExecutionTraces.md), and the [exact shader program catalog](Features/ShaderRuntime/ShaderProgramCatalog.md)

## Module Documentation

| Document | Responsibility |
| --- | --- |
| [Renderer module route](README.md) | reader-first navigation by frame, feature, exact inventory, evidence, or release question |
| [Rendering a Sparkle Frame](RenderingASparkleFrame.md) | canonical current frame narrative from submission through GPU retirement |
| [Renderer Feature Dossiers](Features/README.md) | explicit feature-family definitions, algorithms, limits, decisions, and evidence routes |

The Renderer root intentionally owns only this module route, the canonical frame narrative, and the exact capability ledger. Current feature behavior, feature-local target architectures, catalogs, and substantial feature-local acceptance contracts live under [Features](Features/README.md). Plans remain under `Docs/Plans`; candidate results, cross-feature workloads, and release-wide acceptance remain under `Docs/Acceptance`.

The [RHI module](../RHI/README.md) owns backend-neutral GPU contracts and backend implementations. Cross-module graphics comparisons and end-to-end traces live under [CrossModule](../../../CrossModule/README.md).

## Ownership And Frame Production

| Capability ID | Capability | State | Exact current coverage and limit | Evidence | Release disposition |
| --- | --- | --- | --- | --- | --- |
| `REN-OWN-01` | Public renderer facade | Implemented path | Viewport render requests, immutable submitted frames, UI packets, settings, simulation-frame latency markers, viewport products, shader reload/generation, mesh/texture/memory diagnostics, mesh preview, capture, and render entry points are exposed through the Renderer module. | `S` | Pending |
| `REN-OWN-02` | Scene ownership | Implemented path | `RenderScene` is the persistent render-side mirror. It consumes structural/dynamic deltas and owns render meshes, materials, textures, lights, GPU-scene data, ray-tracing scene state, and history-invalidating scene resets. | `S` | Pending |
| `REN-OWN-03` | View ownership | Implemented path | A prepared `RenderView` owns view/projection-derived state, viewport/output selection, presentation/debug choices, exposure/upscaling settings, and per-view history inputs. | `S` | Pending |
| `REN-OWN-04` | Frame ownership | Implemented path | `FramePipeline` sequences one-way submitted data through scene update, preparation, graph selection/build, execution, provider work, presentation, and retirement. Non-monotonic frame identities are rejected. | `S` | Pending |
| `REN-OWN-05` | Threaded or serial execution | Implemented path | `RenderCoordinator` can run serially or on a render thread; bounded frame/control queues, completions, and execution context separate simulation submission from rendering. | `S` | Pending |
| `REN-OWN-06` | Renderer/RHI boundary | Implemented path | Renderer owns scene/view/frame policy and graph scheduling. RHI owns backend objects, recording, synchronization, presentation, diagnostics, and native interop. | `S` | Pending |

## Frame Graph And GPU Scheduling

| Capability ID | Capability | State | Exact current coverage and limit | Evidence | Release disposition |
| --- | --- | --- | --- | --- | --- |
| `REN-FG-01` | Typed graph authoring | Implemented path | Draw, Dispatch, DispatchAsync, and TraceRays declarations carry typed shader parameters and resource-use declarations into compilation/execution. | `S` | Pending |
| `REN-FG-02` | Compiled pass kinds | Implemented path | Raster, Compute, RayTracing, Transfer, and ExternalProvider are the current pass-kind vocabulary; async compute is a queue preference for compute work rather than a second compute semantic. | `S` | Pending |
| `REN-FG-03` | Resource model | Implemented path | Imported, persistent, transient, history, and exported textures/buffers plus acceleration-structure registrations. | `S` | Pending |
| `REN-FG-04` | Dependency compilation | Implemented path | Producer/consumer dependencies, pass order, queue assignment, state transitions, UAV barriers, and alias barriers are compiled before execution. | `S` | Pending |
| `REN-FG-05` | Transient aliasing | Implemented path | Lifetimes feed transient placement and alias plans; RHI transient blocks materialize the plan. Memory savings and fragmentation behavior are unmeasured. | `S` | Pending |
| `REN-FG-06` | Multi-queue submission | Implemented path | Graphics/compute/copy recording and submission tokens support cross-queue waits. Exposure has an asynchronous-compute path. Actual overlap/performance benefit is not proven. | `S` | Pending |
| `REN-FG-07` | Parallel command recording | Implemented path | A recording plan groups independent chunks for task-based recording before ordered submission. Scaling, determinism, and failure behavior remain runtime evidence items. | `S` | Pending |
| `REN-FG-08` | Graph rebuild and retirement | Implemented path | Extent, output, provider, lighting, GBuffer frontend, shader generation, and shader-table-plan changes invalidate graph products; retired graph/provider/shader resources wait on recorded queue submissions. | `S` | Pending |

## Pipeline Materialization And Typed Binding

| Capability ID | Capability | State | Exact current coverage and limit | Evidence | Release disposition |
| --- | --- | --- | --- | --- | --- |
| `REN-PIPE-01` | Typed pass parameter contract | Implemented path | Registered C++ parameter structures describe uniform, texture/buffer, sampler, acceleration-structure, push-constant, and attachment fields; their computed signature must match cooked shader-map metadata. | `S` | Pending |
| `REN-PIPE-02` | Runtime shader and binding-layout validation | Implemented path | Active map/library entries are checked for type, entry, stage, features, ray metadata, local-record contract, and parameter signature before an RHI binding layout is created. Unsupported RT/inline/descriptor-indexing requirements are rejected. | `S` | Pending |
| `REN-PIPE-03` | Graphics and compute materialization | Implemented path | Compute runtimes are typed per shader/generation. Graphics pipelines are cached by shader generation/code, binding layout, blend/raster/depth/stencil/topology/vertex input, attachment formats, and sample count. Cache/native correctness is unproved. | `S` | Pending |
| `REN-PIPE-04` | Native ray-pipeline and table materialization | Capability-gated | Registered raygen/miss/hit/callable composition becomes an RHI pipeline and table; scene tables require a valid plan plus exactly one opaque and one alpha-tested hit-group composition. | `S` | Pending |
| `REN-PIPE-05` | Atomic shader-generation replacement | Implemented path | Reload opens/validates a complete generation, recreates every already-materialized holder, preserves the active generation on failure, swaps only after success, and retires the prior generation after all queue tokens complete. | `S` | Pending |
| `REN-DIAG-08` | Shader hot reload | Implemented path | A newly published shader map/library is fully validated and materialized before generation swap; prior pipelines/programs retire after their queue submissions complete. This stable ID predates the pipeline-family split and remains here with its semantic owner. | `S` | Pending |

## Scene Preparation, Geometry, And Residency

| Capability ID | Capability | State | Exact current coverage and limit | Evidence | Release disposition |
| --- | --- | --- | --- | --- | --- |
| `REN-SCENE-01` | Persistent GPU scene | Implemented path | Geometry, material, light, and ray-hit payloads are derived from the persistent render scene and uploaded as per-frame bindings. | `S` | Pending |
| `REN-SCENE-02` | Parallel scene preparation | Implemented path | Transformed bounds, joint matrices, morph weights, and lights are prepared through the task system before the ray-tracing work plan. | `S` | Pending |
| `REN-SCENE-03` | Static meshes | Implemented path | Indexed triangle meshes are cached, uploaded, batched, drawn through raster GBuffer, and represented in ray-tracing geometry. | `S` | Pending |
| `REN-SCENE-04` | Skeletal meshes | Implemented path | Skinning supports up to eight influences; current and previous joint matrices feed deformation and motion-vector production. | `S` | Pending |
| `REN-SCENE-05` | Morph targets | Implemented path | Morph weights and GPU morph buffers contribute to raster deformation and motion; ray-tracing vertex positions are also updated. | `S` | Pending |
| `REN-SCENE-06` | Ray-traced deforming geometry | Partial | Skinned/morphed ray-tracing positions are prepared on CPU, a dynamic vertex buffer is replaced, and the BLAS is fully rebuilt for each deforming instance. No BLAS refit/update producer was found. | `S` | Pending |
| `REN-SCENE-07` | Static BLAS reuse | Implemented path | Unchanged static geometry reuses cached bottom-level acceleration structures. | `S` | Pending |
| `REN-SCENE-08` | Mesh residency | Implemented path | Mesh-owned residency defaults to 16 concurrent preparations, 256 requests, 512 MiB decoded, 256 MiB pending upload, and 2 GiB resident; activation and eviction wait on submission completion. Limits are fixed per cache, and pressure/stall/fallback behavior is unproved. | `S` | Pending |
| `REN-SCENE-09` | Texture residency | Implemented path | Texture-owned residency uses the same separate defaults with 16 concurrent loads; cooked read/decode, upload, binding revision, generation replacement, and completion-safe eviction are implemented. Default textures use a synchronous bootstrap route; global memory-pressure behavior is absent. | `S` | Pending |
| `REN-SCENE-10` | Automatic mesh batching | Implemented path | A public rendering setting selects automatic mesh batching. Visual identity and CPU/GPU benefit need workload evidence. | `S` | Pending |

## Visibility And Draw Preparation

| Capability ID | Capability | State | Exact current coverage and limit | Evidence | Release disposition |
| --- | --- | --- | --- | --- | --- |
| `REN-VIS-01` | Per-view frustum visibility | Implemented path | Prepared primitive world AABBs are tested against the current view frustum in a bounded parallel task graph. Invalid bounds conservatively remain visible. Correctness and cost are unproved. | `S` | Pending |
| `REN-VIS-02` | Material visibility classification | Partial | Alpha modes classify as opaque, alpha-tested, transparent, or rejected before batching. Opaque/alpha-tested feed the established deferred path; transparent classification and ordering do not constitute complete blended-transparency support. | `S` | Pending |
| `REN-VIS-03` | Raster candidate validation | Implemented path | Draw index, GPU mesh, instance-group range, material binding, and classification are checked before a candidate enters a batch. Normal view preparation disables optional detailed batch diagnostics. | `S` | Pending |
| `REN-VIS-04` | Authored/preserved groups | Implemented path | Compatible non-transparent authored and shared-mesh-reference groups with at least two visible items remain grouped; incompatible groups fall back to ordinary batching. | `S` | Pending |
| `REN-VIS-05` | Opaque sorting and auto batching | Implemented path | Remaining opaque/alpha-tested work is stably sorted by complete current batch key plus object tie-break and contiguous equal keys batch when `r.MeshAutoBatching` is enabled. Equivalence and benefit are unproved. | `S` | Pending |
| `REN-VIS-06` | Transparent draw preparation | Partial | Transparent candidates are stable-sorted back-to-front by squared bounds-center distance with object tie-break and emitted as single batches. No complete blend/order-independent/refraction/transmission product is claimed. | `S` | Pending |
| `REN-VIS-07` | Visibility task/failure bounds | Implemented path | Capacity is at least 128 then next-power-of-two of bounded primitive count; grain 64, serial threshold 128, at most 8 partitions. Task failure clears indices, batches, and workload rather than publishing a prefix. | `S` | Pending |
| `REN-VIS-08` | Visibility/batch workload facts | Partial | View workload records static/skinned visible instances and batches; optional builder diagnostics can count rejection/group/batch details and estimated saved draws, but the normal path does not collect them. | `S` | Pending |
| `REN-VIS-09` | Advanced visibility/draw generation | Not found | No occlusion/HZB, portal, cluster/meshlet, LOD selection, GPU-driven/indirect/multi-draw, stereo-instancing, or multiview route was found. | `S` | Excluded unless later admitted |

## Temporal Sampling And History

| Capability ID | Capability | State | Exact current coverage and limit | Evidence | Release disposition |
| --- | --- | --- | --- | --- | --- |
| `REN-TEMP-01` | Per-view previous-camera state | Implemented path | `RenderViewState` owns previous matrices/pose/jitter independently for the active view identity and publishes them only with a valid common-history flag. Multi-view isolation is unproved. | `S` | Pending |
| `REN-TEMP-02` | Active temporal jitter | Implemented path | The active path uses a deterministic base-2/base-3 Halton sequence over 16 frames and converts centered samples to NDC with an inverted Y sign. MSAA/R2/white-noise/None implementations are source vocabulary without a current selector. | `S` | Pending |
| `REN-TEMP-03` | Common history invalidation | Implemented path | View identity, scene/shader/provider/topology generation, explicit cut/teleport/reset, inferred camera discontinuity, and projection change invalidate common history and reset narrower lighting/RT planning state. | `S` | Pending |
| `REN-TEMP-04` | Motion and reprojection convention | Implemented path | Raster position is jittered; motion vectors are emitted unjittered; ReSTIR reprojection applies the current-to-previous jitter-grid delta; invalid history emits no geometric motion history. Cross-consumer numerical parity is unproved. | `S` | Pending |
| `REN-TEMP-05` | Provider temporal constants | Capability-gated | Streamline receives pixel-space jitter, previous/current transforms, unjittered-motion declaration, and reset state from the same view temporal uniform. Provider/backend/resize behavior is unproved. | `S` | Pending |

## Resolution, Sampling, And Anti-Aliasing

| Capability ID | Capability | State | Exact current coverage and limit | Evidence | Release disposition |
| --- | --- | --- | --- | --- | --- |
| `REN-RESO-01` | Output extent resolution | Implemented path | A valid viewport request extent wins; otherwise `FramePipeline` uses the window extent. Output extent enters graph settings and swapchain/offscreen product selection. Boundary and resize behavior are unproved. | `S` | Pending |
| `REN-RESO-02` | Render extent resolution | Implemented/capability-gated | With no external provider the render extent equals output extent. Active DLSS SR/RR providers may resolve an optimal internal extent from output extent and quality. Requested/active extent reporting is incomplete. | `S` | Pending |
| `REN-RESO-03` | Extent-driven topology and history | Implemented path | Render/output extent differences enter graph resources, view state, provider inputs and topology comparison; resize/topology invalidates common/provider history and retires old graph/provider generations by completion. | `S` | Pending |
| `REN-RESO-04` | Active raster sample count | Implemented path | Current Renderer graph/pass attachment descriptions resolve to the default single-sample path and materialized pipelines match attachment sample count. A complete enumeration/runtime proof remains open. | `S` | Pending |
| `REN-RESO-05` | Renderer MSAA | Not found | RHI resource/pipeline validation admits sample counts 1/2/4/8 and a source-only MSAA jitter table exists, but no Renderer selector, multisampled pass topology, compatible resolve, or active `RenderViewState` selection was found. | `S` | Excluded unless later admitted |
| `REN-RESO-06` | Standalone post-process AA | Not found | No independently selected TAA, FXAA, or SMAA pass/algorithm was found. The active Halton jitter and DLSS `NativeAA` quality mode must not be advertised as a generic standalone AA implementation. | `S` | Excluded unless later admitted |
| `REN-RESO-07` | Dynamic resolution | Not found | No frame-time controller, automatic resolution scaler, target budget, min/max percentage, hysteresis, selector, or telemetry route was found. Provider-chosen extent for a requested quality mode is not dynamic resolution. | `S` | Excluded unless later admitted |

## Deferred Material And GBuffer Contract

### Material Components Consumed Today

| Capability ID | Component | Authored/GPU coverage | Raster GBuffer | Ray GBuffer and ray/path consumers |
| --- | --- | --- | --- | --- |
| `REN-MAT-01` | Base color | Factor plus texture | Yes | Yes |
| `REN-MAT-02` | Tangent-space normal | Texture-driven normal mapping | Yes | Yes |
| `REN-MAT-03` | Roughness | Factor plus texture | Yes | Yes |
| `REN-MAT-04` | Metallic | Factor plus texture | Yes | Yes |
| `REN-MAT-05` | Ambient occlusion | Factor plus texture | Yes | Yes |
| `REN-MAT-06` | Dielectric F0 | Material value in the GBuffer contract | Yes | Yes |
| `REN-MAT-07` | Emissive | Factor plus texture | Yes | Yes |
| `REN-MAT-08` | Subsurface | Color and strength, each with material/texture contribution | Yes | Yes |
| `REN-MAT-09` | Alpha mask | Alpha mode/cutoff with shader discard/any-hit rejection | Yes | Yes |
| `REN-MAT-10` | Double-sided | Culling/normal orientation semantics | Yes | Yes |

No complete clear-coat, sheen, transmission/refraction, authored index-of-refraction, or anisotropic-material-lobe path was found in this inventory. Those terms must not be included under a generic “PBR supported” claim.

### GBuffer Outputs

| Capability ID | Output | Stored semantics |
| --- | --- | --- |
| `REN-GBUF-01` | Base color | Surface base color plus alpha payload used by the material path. |
| `REN-GBUF-02` | Normal | Shading normal in the renderer's shared GBuffer convention. |
| `REN-GBUF-03` | Material | Metallic, roughness, ambient occlusion, and dielectric F0. |
| `REN-GBUF-04` | Emissive | Surface emissive contribution. |
| `REN-GBUF-05` | Subsurface | Subsurface color and strength. |
| `REN-GBUF-06` | Motion vector | Current-to-previous motion for rigid and deforming geometry; sky motion has a dedicated pass. |
| `REN-GBUF-07` | Device depth | Depth attachment produced by the GBuffer frontend. |
| `REN-GBUF-08` | Scene depth | Linearized depth derived after GBuffer production. |

### Frontends And Limits

| Capability ID | Capability | State | Exact current coverage and limit | Evidence | Release disposition |
| --- | --- | --- | --- | --- | --- |
| `REN-FRONT-01` | Raster GBuffer | Implemented path | Vertex/pixel path, opaque depth-writing draw, solid/wireframe raster state, alpha mask, double-sided handling, current/previous deformation, and eight bindful material texture roles with defaults. | `S` | Pending |
| `REN-FRONT-02` | Ray GBuffer: inline | Capability-gated | Compute shader with inline ray query writes the shared GBuffer outputs and uses the fixed 4096-entry material texture table. | `S` | Pending |
| `REN-FRONT-03` | Ray GBuffer: native pipeline | Capability-gated | Ray-generation, miss, opaque closest-hit, alpha-tested closest-hit, and any-hit programs write the shared outputs through RHI pipeline/SBT/TraceRays. | `S` | Pending |
| `REN-FRONT-04` | Automatic ray execution | Implemented path | Automatic mode prefers native pipeline when supported and otherwise selects inline traversal. Strict mode selection can reject graph creation rather than silently manufacture the requested result. | `S` | Pending |
| `REN-FRONT-05` | Transparent blending | Partial | Blend alpha is represented in material data, but raster GBuffer uses opaque blending and ray hit-group classification is opaque versus alpha-tested. No complete sorted/OIT/transmissive surface path was found. | `S` | Pending |
| `REN-FRONT-06` | Wireframe | Partial | Wireframe changes raster GBuffer fill mode. It is not an equivalent ray-GBuffer visualization mode. | `S` | Pending |
| `REN-FRONT-07` | Material binding mode selector | Vocabulary only | `r.Material.BindingMode` exposes `RayTracingOnly` and `Everything` and defaults to `RayTracingOnly`, but the inspected Renderer has no consumer of this CVar; the capability report advertises only `RayTracingOnly`. `Everything` must not be presented as an active feature. | `S` | Pending |

## Surface Shading And Analytic Light Inputs

| Capability ID | Capability | State | Exact current coverage and limit | Evidence | Release disposition |
| --- | --- | --- | --- | --- | --- |
| `REN-PBR-01` | Active direct specular BRDF | Implemented path | Cook-Torrance microfacet evaluation with the current compile-time defaults: GGX distribution, Smith GGX correlated geometry, and Schlick Fresnel. | `S` | Pending |
| `REN-PBR-02` | Active direct diffuse BRDF | Implemented path | Burley diffuse is the current compile-time default. | `S` | Pending |
| `REN-PBR-03` | Active subsurface approximation | Implemented path | Wrap lighting is the current compile-time default and consumes GBuffer subsurface color/strength. | `S` | Pending |
| `REN-PBR-04` | Alternate BRDF implementations | Vocabulary only | Beckmann/Blinn-Phong distribution; other geometry choices; Lambert/Oren-Nayar/Chan diffuse; and none/Disney subsurface implementations exist behind preprocessor selection, but no registered permutation or public/runtime selector was found. | `S` | Pending |
| `REN-PBR-05` | Indirect approximation helpers | Implemented path | Split-sum style specular/BRDF helpers and the active Jimenez multibounce-AO default are present for applicable indirect paths; specular-occlusion defaults to none. Other preprocessor choices are not a public feature. | `S` | Pending |
| `REN-PBR-06` | Directional lights | Implemented path | GPU-scene payload and direct-light sampling; inspected capacity is 2. | `S` | Pending |
| `REN-PBR-07` | Point lights | Implemented path | Position/range/intensity and shadow eligibility; inspected capacity is 1024. | `S` | Pending |
| `REN-PBR-08` | Spot lights | Implemented path | Position/direction/range/cone/intensity and shadow eligibility; inspected capacity is 1024. | `S` | Pending |
| `REN-PBR-09` | Rect lights | Implemented path | Position/orientation/size/intensity and shadow eligibility; inspected capacity is 1024. | `S` | Pending |
| `REN-PBR-10` | Non-ray lighting fallback | Not found | Current direct visibility and both lighting modes depend on acceleration-structure/ray features. No shadow-map or fully non-ray fallback lighting pipeline was found in the current frame-graph construction. | `S` | Pending |

Capacities above are hard implementation limits from this snapshot, not recommended content budgets or measured performance limits.

## Shared Lighting Modes And Composition

| Capability ID | Mode/effect | State | Exact algorithm and traversal coverage | Evidence | Release disposition |
| --- | --- | --- | --- | --- | --- |
| `REN-LGT-01` | ReSTIR path-traced lighting | Capability-gated | Primary GBuffer plus direct-light reservoir generation, temporal reuse, spatial reuse, and ray-traced visibility; indirect reservoir temporal/spatial/resolve; shared lighting composite and sky. | `S` | Pending |
| `REN-LGT-04` | Reference path-traced lighting | Capability-gated | A GBuffer-seeded inline path sample produces direct and indirect radiance into RGBA32F accumulation/history before shared composite/sky. It shares primary surface, material/light/shadow, frame-history, and presentation dependencies; the [completion study](../../../../Research/GraphicsArchitecture/OfflinePathTracerCompletion.md) therefore classifies current output as a candidate comparison, not an accepted unbiased oracle. | `S` | Pending; `PTD-00` |
| `REN-LGT-05` | Accumulation invalidation | Implemented path | Scene, camera/view, settings, extent, and relevant lighting state contribute to reference/ReSTIR history validity. Completeness under every editor action requires runtime testing. | `S` | Pending |
| `REN-LGT-06` | Lighting composite | Implemented path | Direct diffuse, direct specular, direct subsurface, indirect diffuse, indirect specular, and GBuffer emissive are combined before post processing. | `S` | Pending |

## Direct Lighting

| Capability ID | Capability | State | Exact current coverage and limit | Evidence | Release disposition |
| --- | --- | --- | --- | --- | --- |
| `REN-LGT-02` | Direct shadow visibility | Capability-gated | Dual execution: inline ray query or native pipeline. Produces a visibility signal consumed by direct lighting. The four analytic light kinds and active direct BRDF lobes are enumerated under `REN-PBR-*`. | `S` | Pending |

The complete algorithm, light limits, BRDF terms, inputs/outputs, and traversal contract are in [Direct Lighting](Features/Lighting/DirectLighting.md).

## Indirect Lighting

| Capability ID | Capability | State | Exact current coverage and limit | Evidence | Release disposition |
| --- | --- | --- | --- | --- | --- |
| `REN-LGT-03` | ReSTIR indirect | Capability-gated | Inline ray-query path with temporal and spatial reservoir reuse and resolve; configurable bounce count is clamped to 8 in the inspected settings path. No native-pipeline adapter was found. | `S` | Pending |
| `REN-LGT-07` | Sky | Implemented path | Dedicated sky lighting and sky motion-vector production are in the graph. Environment/IBL asset breadth is not claimed by this row. | `S` | Pending |

The complete ReSTIR/reference, history, secondary-transport, environment, and oracle boundary is in [Indirect Lighting](Features/Lighting/IndirectLighting.md).

## Volumetric Lighting

| Capability ID | Capability | State | Exact current coverage and limit | Evidence | Release disposition |
| --- | --- | --- | --- | --- | --- |
| `REN-VOL-01` | Participating media and fog volumes | Not found | No authored/cooked medium or fog-volume record, render-scene/GPU-scene payload, selector, froxel/voxel representation, or integration pass was found. | `S` | Excluded unless later admitted |
| `REN-VOL-02` | Volumetric light transport | Not found | No extinction, absorption, phase-function, in-scattering, transmittance, volumetric shadow, multiple-scattering, temporal reconstruction, or composite product was found. | `S` | Excluded unless later admitted |
| `REN-VOL-03` | Atmosphere and aerial perspective | Not found | Sky background fill exists, but no atmospheric scattering, height fog, aerial perspective, or volumetric-cloud lighting path was found. | `S` | Excluded unless later admitted |

The negative capability boundary and the minimum ownership required for any future proposal are in [Volumetric Lighting](Features/Lighting/VolumetricLighting.md).

## Deferred Decals

| Capability ID | Capability | State | Exact current coverage and limit | Evidence | Release disposition |
| --- | --- | --- | --- | --- | --- |
| `REN-DECAL-01` | Authored and scene decal data | Not found | No component, imported/cooked record, render-scene table, GPU-scene payload, selector, or editor authoring path was found. | `S` | Excluded; future plan only |
| `REN-DECAL-02` | Primary deferred GBuffer composition | Not found | No decal pass modifies the shared raster/ray GBuffer before depth-derived and lighting consumers. | `S` | Excluded; future plan only |
| `REN-DECAL-03` | Secondary-ray decal evaluation | Not found | Arbitrary ray hits evaluate base materials without decal candidate lookup or composition. | `S` | Excluded; future plan only |

Current absence, extension seams, and the separately labeled target architecture are routed by [Deferred Decals](Features/DeferredDecals/README.md).

## Ray-Tracing Scene And Shader Tables

| Capability ID | Capability | State | Exact current coverage and limit | Evidence | Release disposition |
| --- | --- | --- | --- | --- | --- |
| `REN-RT-01` | Classic TLAS | Capability-gated | Per-view classic TLAS build with an optional refit/update setting; instances carry stable scene identity and shader-table contribution. | `S` | Pending |
| `REN-RT-02` | Partitioned TLAS | Capability-gated | Selected through RHI provider capability/preferences with a per-view 3D partition planner. Current build strategy disables instance updates and partition translation and caps operation count at one. | `S` | Pending |
| `REN-RT-03` | Shared scene identity | Implemented path | Classic and partitioned acceleration paths derive from the same prepared scene/GPU-scene identity rather than independent material/effect representations. | `S` | Pending |
| `REN-RT-04` | Shader-table plan | Implemented path | Logical geometry/material/hit-group classification maps to checked SBT record indices for two ray types: surface and shadow visibility. | `S` | Pending |
| `REN-RT-05` | Hit-group coverage | Partial | Opaque and alpha-tested triangle hit groups are authored. Procedural/intersection and callable program coverage was not found in current Renderer effects. | `S` | Pending |
| `REN-RT-06` | Plan invalidation | Implemented path | Generation changes follow geometry/hit-group/SBT semantic changes rather than ordinary material-value edits; retired tables/pipelines wait on queue submissions. | `S` | Pending |

## Exposure

| Capability ID | Capability | State | Exact current coverage and limit | Evidence | Release disposition |
| --- | --- | --- | --- | --- | --- |
| `REN-POST-01` | Manual exposure | Implemented path | Manual exposure, compensation, and bounded exposure values flow through per-view settings and the exposure pass. | `S` | Pending |
| `REN-POST-02` | Automatic exposure | Implemented path | Parallel-reduction and downsample-pyramid metering choices, temporal moments/history, target and asymmetric adaptation speeds. | `S` | Pending |
| `REN-POST-03` | Async exposure | Implemented path | Exposure can be scheduled as asynchronous compute through the frame graph. Actual overlap is unmeasured. | `S` | Pending |

## Image Reconstruction And Upscaling

| Capability ID | Capability | State | Exact current coverage and limit | Evidence | Release disposition |
| --- | --- | --- | --- | --- | --- |
| `REN-POST-04` | Linear upscaler | Implemented path | Built-in spatial linear upscale is the baseline provider/path. Quality and cost need resolution-specific evidence. | `S` | Pending |
| `REN-POST-05` | NVIDIA DLSS Super Resolution | Capability-gated | Optional Streamline-backed provider with NativeAA, Quality, Balanced, Performance, and UltraPerformance selections. Provider initialization failure falls back to Linear rather than claiming DLSS output. | `S` | Pending |
| `REN-POST-06` | NVIDIA DLSS Ray Reconstruction | Capability-gated | Optional Streamline-backed reconstruction for the ReSTIR lighting route, consuming color, depth, motion, diffuse/specular albedo, normal, roughness, specular hit distance, and exposure. Initialization failure disables the provider path. | `S` | Pending |

## Tone Mapping

| Capability ID | Capability | State | Exact current coverage and limit | Evidence | Release disposition |
| --- | --- | --- | --- | --- | --- |
| `REN-POST-07` | Tone mapping | Implemented path | Reinhard, ACES approximation, and ACES filmic choices. | `S` | Pending |

## Color Grading

| Capability ID | Capability | State | Exact current coverage and limit | Evidence | Release disposition |
| --- | --- | --- | --- | --- | --- |
| `REN-POST-11` | Color grading | Not found | No grading pass/shader, parameter stack, 1D/3D LUT asset/import/cook route, color-space transform, selector, debug product, or editor workflow was found. The three tone-mapper choices are not grading. | `S` | Excluded unless later admitted |

## Chromatic Aberration

| Capability ID | Capability | State | Exact current coverage and limit | Evidence | Release disposition |
| --- | --- | --- | --- | --- | --- |
| `REN-POST-12` | Chromatic aberration | Not found | No lens/channel distortion model, strength/center/falloff setting, pass, shader, selector, history, diagnostic, or editor workflow was found. Reconstruction artifacts are not feature support. | `S` | Excluded unless later admitted |

## Frame Generation

| Capability ID | Capability | State | Exact current coverage and limit | Evidence | Release disposition |
| --- | --- | --- | --- | --- | --- |
| `REN-POST-13` | Frame generation | Not found | No optical-flow/frame-synthesis provider, DLSS-G registration, generated-frame identity/resources, UI policy, pacing/present path, selector, or diagnostics were found. Streamline PCL/Reflex latency coordination is not frame generation. | `S` | Excluded unless later admitted |

## Presentation And Output

| Capability ID | Capability | State | Exact current coverage and limit | Evidence | Release disposition |
| --- | --- | --- | --- | --- | --- |
| `REN-POST-08` | Output encoding | Implemented path | Automatic, Linear, and sRGB encoding selections. | `S` | Pending |
| `REN-POST-09` | HDR display output | Not found | No PQ, scRGB, HDR10 metadata, display-nit contract, or HDR swapchain mode was found in the current Renderer/RHI output selections. | `S` | Excluded unless later admitted |
| `REN-POST-10` | Exact debug presentation | Partial | Debug views flow through the existing exposure/tone/output chain. The separate exact display-linear versus scene-referred debug presentation described in the target design is not implemented in this snapshot. | `S` | Pending |

## Debug Views

The public `RenderViewMode` surface contains 16 values:

| Capability ID | Category | Modes | Current boundary |
| --- | --- | --- | --- |
| `REN-DBG-01` | Final/material | Lit, Wireframe | Wireframe is raster-frontend state; Lit uses the selected lighting/output pipeline. |
| `REN-DBG-02` | GBuffer | Diffuse/Base Color, Normal, Roughness, Metallic, Emissive, Ambient Occlusion, Subsurface Color, Subsurface Strength | Buffer visualization path; presentation is still subject to current post/output behavior. |
| `REN-DBG-03` | Lighting | Direct Diffuse, Direct Specular, Direct Subsurface, Indirect Diffuse, Indirect Specular | Visualizes individual lighting targets where the active mode produces them. |
| `REN-DBG-04` | Scene diagnostics | GPU Scene Instances | Diagnostic visualization of GPU-scene instance data. |

Each selectable mode needs a representative screenshot/capture, expected-value description, backend record, unsupported-combination behavior, and output-transform check before inclusion.

## UI And Editor Viewport Composition

| Capability ID | Capability | State | Exact current coverage and limit | Evidence | Release disposition |
| --- | --- | --- | --- | --- | --- |
| `REN-UI-01` | UI render packets | Implemented path | Renderer consumes an immutable `UiRenderPacket` after frame-graph execution and before final frame submission; `None`, host-overlay, and editor-viewport presentation modes are explicit. | `S` | Pending |
| `REN-UI-02` | Host overlay | Implemented path | When draw data exists, the packet is replayed through the RHI ImGui renderer inside a presentation overlay pass. Correct blend/color/DPI/input behavior is unverified. | `S` | Pending |
| `REN-UI-03` | Editor viewport presentation | Implemented path | The final viewport scene-color product is transitioned for shader read, resolved to an ImGui texture, checked against viewport generation, drawn inside a present render pass, and transitioned back. Missing product/texture or generation mismatch refuses the draw. | `S` | Pending |
| `REN-UI-04` | Editor texture handles | Partial | A Renderer registry maps viewport-generation handles and other native texture IDs to UI texture bindings. Non-viewport registrations are append-only in the inspected owner; long-session lifecycle/bounds and stale-handle behavior need evidence. | `S` | Pending |

## Latency Coordination

| Capability ID | Capability | State | Exact current coverage and limit | Evidence | Release disposition |
| --- | --- | --- | --- | --- | --- |
| `REN-LAT-01` | Logical frame latency markers | Implemented public path | The facade exposes simulation start/end; D3D12 device services emit render-submit start/end and present start/end around the corresponding frame operations. Correct host bracketing and one-ID ordering are unproved. | `S` | Pending |
| `REN-LAT-02` | Streamline PCL marker route | Capability-gated | When the optional runtime is initialized, device-bound, presentation-ready, and PCL-supported on D3D12, all six markers map to Streamline PCL. Unsupported/unready cells do no provider work. | `S` | Pending |
| `REN-LAT-03` | Reflex simulation sleep | Capability-gated | When Reflex is supported, `slReflexSleep` runs at SimulationStart before the PCL marker. Only default Reflex options are configured; no public latency-mode setting or active-state product was found. | `S` | Pending |
| `REN-LAT-04` | Provider call/shutdown lifetime | Implemented path | Mutex-protected call leases reject work during shutdown; shutdown waits for active calls before resetting runtime/device/presentation/feature state. Boundedness and race safety are unproved. | `S` | Pending |
| `REN-LAT-05` | Frame-token identity | Partial | Public logical IDs are 64-bit but the Streamline token request narrows them to 32-bit. Correct supported run length or wrap enforcement is not established. | `S` | Pending |

## Diagnostics, Capture, Preview, And Hot Reload

| Capability ID | Capability | State | Exact current coverage and limit | Evidence | Release disposition |
| --- | --- | --- | --- | --- | --- |
| `REN-DIAG-01` | Frame/pass diagnostics | Implemented path | Frame execution, pass execution, scoped GPU timing/events, and frame-graph contract diagnostics are collected through Renderer/RHI diagnostics. | `S` | Pending |
| `REN-DIAG-02` | Mesh diagnostics | Implemented path | Mesh cache/residency/detail snapshots are exposed through the public facade. Correctness and usability are unverified. | `S` | Pending |
| `REN-DIAG-03` | Texture diagnostics | Implemented path | Texture-cache/residency snapshots are exposed through the public facade. | `S` | Pending |
| `REN-DIAG-04` | Memory diagnostics | Implemented path | Renderer memory monitor combines relevant resource/cache/RHI budget information for callers. | `S` | Pending |
| `REN-DIAG-05` | Viewport products | Implemented path | Viewports can target the swapchain or offscreen products used by editor/UI consumers. Product lifetime follows frame retirement. | `S` | Pending |
| `REN-DIAG-06` | Async captures | Partial | Requested viewport/final or intermediate products flow through RHI readback and later completion polling. Results record frame, scene, and provider generations, dimensions, row pitch, pixel format, artifact, and failure; shader/graph-topology generation plus requested-versus-resolved product and color/encoding provenance are not carried. Format/color correctness needs runtime proof. | `S` | Pending |
| `REN-DIAG-07` | Mesh preview | Implemented path | Renderer exposes a mesh-preview product/handle route for editor consumers. | `S` | Pending |

## Settings State And Persistence

| Capability ID | Capability | State | Exact current coverage and limit | Evidence | Release disposition |
| --- | --- | --- | --- | --- | --- |
| `REN-SET-01` | Aggregate public settings state | Implemented path | A 28-field value snapshot spans presentation/device, tone/output, exposure, reconstruction/upscaling, GBuffer/RT, lighting, batching/TLAS/PTLAS, and view mode; it records requested values, not resolved provider/capability state. | `S` | Pending |
| `REN-SET-02` | Owned settings persistence | Partial | Twenty-seven CVar names are loaded from and rewrite one owned section in workspace `Config/DefaultEngine.ini`; view mode is session-only. Writes truncate in place and return no error, so durability/concurrent edit/package behavior is not established. | `S` | Pending |
| `REN-SET-03` | Startup and editor commit | Implemented path | Application loads persisted values before command-line CVar overrides. Editor changes persist the whole state and submit it through a bound host callback; malformed parse diagnostics are currently discarded. | `S` | Pending |
| `REN-SET-04` | Serial/threaded settings handoff | Implemented path | Serial coordination applies changed CVars directly; threaded coordination queues the whole snapshot to the render execution context. Ordering, backpressure, shutdown, and next-frame equivalence are unproved. | `S` | Pending |
| `REN-SET-05` | Live versus restart-active state | Partial | Adapter preference and back-buffer format produce a pending-restart message relative to session-start state; other values apply to CVars, but requested/CVar/resolved/session-active state and fallback reason are not unified in one report. | `S` | Pending |

## Public And Developer Selection Surface

The inspected public settings cover VSync, back-buffer format, adapter preference, tone mapper, exposure, output encoding, upscaler and quality, ray reconstruction, GBuffer frontend, ray-tracing execution, lighting mode, automatic mesh batching, classic TLAS refit, partitioned TLAS activation/planning controls, and view mode. Direct-shadow traversal also has a console-variable surface even where it is not mirrored by the same public settings object. `r.BackBufferCount` and `r.MaximumFramesInFlight` are RHI console controls outside the Renderer settings state/persistence. `r.Material.BindingMode` is registered but has no inspected runtime consumer; its `Everything` value is vocabulary, not an active selectable result.

The settings section persists 27 named Renderer CVars in `/Script/SparkleRenderer.EngineRenderingSettings` inside workspace `Config/DefaultEngine.ini`, replacing that section while retaining other sections. View mode is session state and is not in that persisted set. Adapter preference and back-buffer format are written immediately but reported as pending restart; the other section values are applied to Renderer CVars on commit. The full lifecycle and known in-place-write/diagnostic gaps are owned by [Settings State and Persistence](Features/RuntimeConfiguration/SettingsStateAndPersistence.md).

Release inventory must be generated from both UI/public settings and console/config surfaces. A hidden but reachable command or CVar is still a selectable capability unless Shipping erases or locks it.

No current selector exists for Volumetric Lighting, deferred decals, color grading, chromatic aberration, frame generation, HDR display output, or a non-ray lighting fallback because those product paths are absent. The exact current route, defaults, clamps, persistence, known unused selector, and deliberately absent selector set are in the [Feature Selector Catalog](Features/RuntimeConfiguration/FeatureSelectorCatalog.md).

## Explicit Non-Claims And Shipping Risks

- No Renderer target was built and no frame, image, GPU capture, validation layer, or performance workload was run for this snapshot.
- PBR support is limited to the material components and BRDF families listed above. It is not blanket glTF/Disney/MaterialX feature completeness.
- Alpha masking exists; general transparent blending/transmission does not have a complete current render path.
- Ray use currently has no fully non-ray lighting fallback. Minimum hardware/support claims must account for that fact.
- Native RT pipeline execution covers ray GBuffer and direct-shadow visibility, not reference path tracing or ReSTIR indirect.
- Dynamic ray-traced deformation performs CPU position work and full BLAS rebuild; it is not refit/update support and may be expensive.
- Partitioned TLAS has a richer low-level RHI contract than the Renderer currently exercises.
- Optional NVIDIA providers require exact binary, adapter, driver, interposer, feature, and fallback evidence before advertisement.
- Async compute and parallel recording are implemented scheduling paths, not proven speedups.
- Typed parameter validation and pipeline caches are implemented mechanisms, not native pipeline correctness or hot-reload stress evidence.
- The active temporal jitter is the 16-frame Halton route. Other source pattern implementations are not public modes, and common-history correctness is unproved.
- The current Renderer is single-sample at its active raster attachments. RHI 2/4/8 sample-count vocabulary and an unused MSAA jitter helper do not establish MSAA; no standalone TAA/FXAA/SMAA or dynamic-resolution controller was found.
- Visibility is CPU frustum/AABB plus deterministic classification/sort/batch preparation. No occlusion, LOD selection, GPU-driven/indirect draw, stereo, or multiview route was found.
- Mesh and texture caches have separate fixed residency budgets and concurrency limits; they do not constitute global memory-pressure streaming, prioritization, LRU eviction, or graceful degradation.
- Streamline PCL/Reflex is optional, D3D12-only in the inspected route, has no public active-mode report, narrows the 64-bit frame ID for provider tokens, and has no measured latency claim.
- Settings persistence rewrites a workspace INI section in place and reports no write/parse failure; package-safe durable persistence is not established.
- Current debug views do not yet have the target exact display-linear presentation split.
- No current HDR-display output contract was found.
- No participating-media, fog-volume, volumetric-lighting, atmosphere, or aerial-perspective path was found; sky and wrap-subsurface lighting do not imply those capabilities.
- Deferred decals have a target architecture, delivery plan, and acceptance contract, but no current authored data, scene/GPU representation, GBuffer pass, or secondary-ray composition path.
- No color-grading parameters/LUT transform, chromatic-aberration lens effect, or frame-generation synthesis/presentation path was found. Tone mapping, reconstruction artifacts, Streamline PCL, and Reflex are not substitutes.

## Primary Source Routes

- Public facade and settings: `Engine/Renderer/Public`.
- Runtime owner and frame sequencing: `Engine/Renderer/Private/Host`, `Engine/Renderer/Private/Concurrency`, and `Engine/Renderer/Private/Frame`.
- Persistent scene/view preparation: `Engine/Renderer/Private/Scene` and `Engine/Renderer/Private/View`.
- Temporal sampling/history: `Engine/Renderer/Private/Temporal`, `Engine/Renderer/Private/View/RenderViewState.*`, `Engine/Renderer/Private/Resources/History`, and temporal shader/provider consumers.
- Frame graph: `Engine/Renderer/Private/FrameGraph`.
- Pipeline materialization/binding: `Engine/Renderer/Private/Pipeline`, `Engine/Renderer/Private/PipelineRuntime`, `Engine/Renderer/Private/ShaderParameters`, and `Engine/Renderer/Public/ShaderParameters`.
- Geometry/residency: `Engine/Renderer/Private/Meshes`, `Engine/Renderer/Private/Textures`, and `Engine/Renderer/Private/Scene/GpuScene`.
- Raster/ray GBuffer: `Engine/Renderer/Private/Passes/GBuffer`.
- Lighting: `Engine/Renderer/Private/Passes/Lighting` and `Engine/Assets/Shaders/Lighting`, `Engine/Assets/Shaders/BRDF`, and `Engine/Assets/Shaders/Passes/Lighting`.
- Ray scene and execution: `Engine/Renderer/Private/Scene/RayTracing`, `Engine/Renderer/Private/RayTracing`, and `Engine/Renderer/Private/Passes/Lighting/Shadows`.
- Post/output/providers: `Engine/Renderer/Private/Passes/PostProcessing`, `Engine/Renderer/Private/Passes/Presentation`, and `Engine/Renderer/Private/Providers`.
- Diagnostics/editor products: `Engine/Renderer/Private/Diagnostics`, `Engine/Renderer/Private/Editor`, and public Renderer diagnostic/product contracts.
- Settings lifecycle: `Engine/Renderer/Private/Settings`, public settings contracts, `Engine/Application` startup, and `Engine/Editor` commit wiring.
- Latency coordination: `Engine/Renderer/Private/Integrations`, `Engine/Renderer/Private/Streamline`, Renderer facade simulation calls, and RHI frame-marker hooks.
- Build and shader registration membership: `Engine/Renderer/CMakeLists.txt`.
