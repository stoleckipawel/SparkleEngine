# Renderer Capability Inventory

Status: capability snapshot; not release approval, visual validation, or performance evidence

Snapshot: 2026-09-06 at committed `master` revision `8414b5dc`; current `Engine/Renderer` public surface, CMake membership, Scene/View/Frame path, frame graph, passes, ray-tracing paths, providers, and shader registrations inspected; evidence `S` only

Scope: render-side ownership, frame production, geometry/material/lighting algorithms, ray execution, post processing, providers, debug products, diagnostics, and known coverage gaps

Owner: `Engine/Renderer`

Supporting maps: [Whole Repository Architecture Map](../../../WholeRepositoryMap.md), [Renderer and RHI Architecture Boundary](../../../Decisions/RendererRhiBoundary.md), and [Ray-Tracing Execution System](RayTracingExecution.md)

Evidence plan and release disposition: [Capability Evidence Plan](../../../../Plans/CapabilityEvidence.md) and [First Release Acceptance Contract](../../../../Acceptance/FirstRelease.md)

Deeper routes: [cross-system graphics coverage](../../../CrossModule/GraphicsCoverageMatrix.md), [producer-to-consumer execution traces](../../../CrossModule/FeatureExecutionTraces.md), and the [exact shader program catalog](ShaderProgramCatalog.md)

## Module Documentation

| Document | Responsibility |
| --- | --- |
| [Debug View Presentation](DebugViewPresentation.md) | renderer-owned presentation and show-flag architecture |
| [Debug View Presentation Capability](DebugViewPresentationCapability.md) | dated source snapshot for the existing debug-view path |
| [Deferred GBuffer Decals](DeferredGBufferDecals.md) | renderer-owned target decal composition architecture |
| [Deferred GBuffer Decals Capability](DeferredGBufferDecalsCapability.md) | dated source snapshot for current decal-related seams and gaps |
| [Ray-Tracing Execution](RayTracingExecution.md) | Renderer/RHI execution contract with Renderer as the semantic effect owner |
| [Shader Program Catalog](ShaderProgramCatalog.md) | exact catalog of registered Renderer shader programs and consumers |

The [RHI module](../RHI/README.md) owns backend-neutral GPU contracts and backend implementations. Cross-module graphics comparisons and end-to-end traces live under [CrossModule](../../../CrossModule/README.md).

## Ownership And Frame Production

| Capability | State | Exact current coverage and limit | Evidence | Release disposition |
| --- | --- | --- | --- | --- |
| Public renderer facade | Implemented path | Viewport render requests, immutable submitted frames, UI packets, settings, viewport products, shader reload/generation, mesh/texture/memory diagnostics, mesh preview, capture, and render entry points are exposed through the Renderer module. | `S` | Pending |
| Scene ownership | Implemented path | `RenderScene` is the persistent render-side mirror. It consumes structural/dynamic deltas and owns render meshes, materials, textures, lights, GPU-scene data, ray-tracing scene state, and history-invalidating scene resets. | `S` | Pending |
| View ownership | Implemented path | A prepared `RenderView` owns view/projection-derived state, viewport/output selection, presentation/debug choices, exposure/upscaling settings, and per-view history inputs. | `S` | Pending |
| Frame ownership | Implemented path | `FramePipeline` sequences one-way submitted data through scene update, preparation, graph selection/build, execution, provider work, presentation, and retirement. Non-monotonic frame identities are rejected. | `S` | Pending |
| Threaded or serial execution | Implemented path | `RenderCoordinator` can run serially or on a render thread; bounded frame/control queues, completions, and execution context separate simulation submission from rendering. | `S` | Pending |
| Renderer/RHI boundary | Implemented path | Renderer owns scene/view/frame policy and graph scheduling. RHI owns backend objects, recording, synchronization, presentation, diagnostics, and native interop. | `S` | Pending |

## Frame Graph And GPU Scheduling

| Capability | State | Exact current coverage and limit | Evidence | Release disposition |
| --- | --- | --- | --- | --- |
| Typed graph authoring | Implemented path | Draw, Dispatch, DispatchAsync, and TraceRays declarations carry typed shader parameters and resource-use declarations into compilation/execution. | `S` | Pending |
| Compiled pass kinds | Implemented path | Raster, Compute, RayTracing, Transfer, and ExternalProvider are the current pass-kind vocabulary; async compute is a queue preference for compute work rather than a second compute semantic. | `S` | Pending |
| Resource model | Implemented path | Imported, persistent, transient, history, and exported textures/buffers plus acceleration-structure registrations. | `S` | Pending |
| Dependency compilation | Implemented path | Producer/consumer dependencies, pass order, queue assignment, state transitions, UAV barriers, and alias barriers are compiled before execution. | `S` | Pending |
| Transient aliasing | Implemented path | Lifetimes feed transient placement and alias plans; RHI transient blocks materialize the plan. Memory savings and fragmentation behavior are unmeasured. | `S` | Pending |
| Multi-queue submission | Implemented path | Graphics/compute/copy recording and submission tokens support cross-queue waits. Exposure has an asynchronous-compute path. Actual overlap/performance benefit is not proven. | `S` | Pending |
| Parallel command recording | Implemented path | A recording plan groups independent chunks for task-based recording before ordered submission. Scaling, determinism, and failure behavior remain runtime evidence items. | `S` | Pending |
| Graph rebuild and retirement | Implemented path | Extent, output, provider, lighting, GBuffer frontend, shader generation, and shader-table-plan changes invalidate graph products; retired graph/provider/shader resources wait on recorded queue submissions. | `S` | Pending |

## Scene Preparation, Geometry, And Residency

| Capability | State | Exact current coverage and limit | Evidence | Release disposition |
| --- | --- | --- | --- | --- |
| Persistent GPU scene | Implemented path | Geometry, material, light, and ray-hit payloads are derived from the persistent render scene and uploaded as per-frame bindings. | `S` | Pending |
| Parallel scene preparation | Implemented path | Transformed bounds, joint matrices, morph weights, and lights are prepared through the task system before the ray-tracing work plan. | `S` | Pending |
| Static meshes | Implemented path | Indexed triangle meshes are cached, uploaded, batched, drawn through raster GBuffer, and represented in ray-tracing geometry. | `S` | Pending |
| Skeletal meshes | Implemented path | Skinning supports up to eight influences; current and previous joint matrices feed deformation and motion-vector production. | `S` | Pending |
| Morph targets | Implemented path | Morph weights and GPU morph buffers contribute to raster deformation and motion; ray-tracing vertex positions are also updated. | `S` | Pending |
| Ray-traced deforming geometry | Partial | Skinned/morphed ray-tracing positions are prepared on CPU, a dynamic vertex buffer is replaced, and the BLAS is fully rebuilt for each deforming instance. No BLAS refit/update producer was found. | `S` | Pending |
| Static BLAS reuse | Implemented path | Unchanged static geometry reuses cached bottom-level acceleration structures. | `S` | Pending |
| Mesh residency | Implemented path | Asynchronous mesh preparation/upload/residency tracking is bounded to 16 concurrent preparations in the inspected implementation. Budget/stall behavior is unmeasured. | `S` | Pending |
| Texture residency | Implemented path | Asynchronous read/decode/upload/residency tracking is bounded to 16 concurrent loads in the inspected implementation. The limit is concurrency, not total decoded-memory capacity. | `S` | Pending |
| Automatic mesh batching | Implemented path | A public rendering setting selects automatic mesh batching. Visual identity and CPU/GPU benefit need workload evidence. | `S` | Pending |

## Deferred Material And GBuffer Contract

### Material components consumed today

| Component | Authored/GPU coverage | Raster GBuffer | Ray GBuffer and ray/path consumers |
| --- | --- | --- | --- |
| Base color | Factor plus texture | Yes | Yes |
| Tangent-space normal | Texture-driven normal mapping | Yes | Yes |
| Roughness | Factor plus texture | Yes | Yes |
| Metallic | Factor plus texture | Yes | Yes |
| Ambient occlusion | Factor plus texture | Yes | Yes |
| Dielectric F0 | Material value in the GBuffer contract | Yes | Yes |
| Emissive | Factor plus texture | Yes | Yes |
| Subsurface | Color and strength, each with material/texture contribution | Yes | Yes |
| Alpha mask | Alpha mode/cutoff with shader discard/any-hit rejection | Yes | Yes |
| Double-sided | Culling/normal orientation semantics | Yes | Yes |

No complete clear-coat, sheen, transmission/refraction, authored index-of-refraction, or anisotropic-material-lobe path was found in this inventory. Those terms must not be included under a generic “PBR supported” claim.

### GBuffer outputs

| Output | Stored semantics |
| --- | --- |
| Base color | Surface base color plus alpha payload used by the material path. |
| Normal | Shading normal in the renderer's shared GBuffer convention. |
| Material | Metallic, roughness, ambient occlusion, and dielectric F0. |
| Emissive | Surface emissive contribution. |
| Subsurface | Subsurface color and strength. |
| Motion vector | Current-to-previous motion for rigid and deforming geometry; sky motion has a dedicated pass. |
| Device depth | Depth attachment produced by the GBuffer frontend. |
| Scene depth | Linearized depth derived after GBuffer production. |

### Frontends and limits

| Capability | State | Exact current coverage and limit | Evidence | Release disposition |
| --- | --- | --- | --- | --- |
| Raster GBuffer | Implemented path | Vertex/pixel path, opaque depth-writing draw, solid/wireframe raster state, alpha mask, double-sided handling, current/previous deformation, and eight bindful material texture roles with defaults. | `S` | Pending |
| Ray GBuffer: inline | Capability-gated | Compute shader with inline ray query writes the shared GBuffer outputs and uses the fixed 4096-entry material texture table. | `S` | Pending |
| Ray GBuffer: native pipeline | Capability-gated | Ray-generation, miss, opaque closest-hit, alpha-tested closest-hit, and any-hit programs write the shared outputs through RHI pipeline/SBT/TraceRays. | `S` | Pending |
| Automatic ray execution | Implemented path | Automatic mode prefers native pipeline when supported and otherwise selects inline traversal. Strict mode selection can reject graph creation rather than silently manufacture the requested result. | `S` | Pending |
| Transparent blending | Partial | Blend alpha is represented in material data, but raster GBuffer uses opaque blending and ray hit-group classification is opaque versus alpha-tested. No complete sorted/OIT/transmissive surface path was found. | `S` | Pending |
| Wireframe | Partial | Wireframe changes raster GBuffer fill mode. It is not an equivalent ray-GBuffer visualization mode. | `S` | Pending |

## PBR Shading And Lights

| Capability | State | Exact current coverage and limit | Evidence | Release disposition |
| --- | --- | --- | --- | --- |
| Active direct specular BRDF | Implemented path | Cook-Torrance microfacet evaluation with the current compile-time defaults: GGX distribution, Smith GGX correlated geometry, and Schlick Fresnel. | `S` | Pending |
| Active direct diffuse BRDF | Implemented path | Burley diffuse is the current compile-time default. | `S` | Pending |
| Active subsurface approximation | Implemented path | Wrap lighting is the current compile-time default and consumes GBuffer subsurface color/strength. | `S` | Pending |
| Alternate BRDF implementations | Vocabulary only | Beckmann/Blinn-Phong distribution; other geometry choices; Lambert/Oren-Nayar/Chan diffuse; and none/Disney subsurface implementations exist behind preprocessor selection, but no registered permutation or public/runtime selector was found. | `S` | Pending |
| Indirect approximation helpers | Implemented path | Split-sum style specular/BRDF helpers and the active Jimenez multibounce-AO default are present for applicable indirect paths; specular-occlusion defaults to none. Other preprocessor choices are not a public feature. | `S` | Pending |
| Directional lights | Implemented path | GPU-scene payload and direct-light sampling; inspected capacity is 2. | `S` | Pending |
| Point lights | Implemented path | Position/range/intensity and shadow eligibility; inspected capacity is 1024. | `S` | Pending |
| Spot lights | Implemented path | Position/direction/range/cone/intensity and shadow eligibility; inspected capacity is 1024. | `S` | Pending |
| Rect lights | Implemented path | Position/orientation/size/intensity and shadow eligibility; inspected capacity is 1024. | `S` | Pending |
| Non-ray lighting fallback | Not found | Current direct visibility and both lighting modes depend on acceleration-structure/ray features. No shadow-map or fully non-ray fallback lighting pipeline was found in the current frame-graph construction. | `S` | Pending |

Capacities above are hard implementation limits from this snapshot, not recommended content budgets or measured performance limits.

## Lighting Modes

| Mode/effect | State | Exact algorithm and traversal coverage | Evidence | Release disposition |
| --- | --- | --- | --- | --- |
| ReSTIR path-traced lighting | Capability-gated | Primary GBuffer plus direct-light reservoir generation, temporal reuse, spatial reuse, and ray-traced visibility; indirect reservoir temporal/spatial/resolve; shared lighting composite and sky. | `S` | Pending |
| Direct shadow visibility | Capability-gated | Dual execution: inline ray query or native pipeline. Produces a visibility signal consumed by direct lighting. | `S` | Pending |
| ReSTIR indirect | Capability-gated | Inline ray-query path with temporal and spatial reservoir reuse and resolve; configurable bounce count is clamped to 8 in the inspected settings path. No native-pipeline adapter was found. | `S` | Pending |
| Reference path-traced lighting | Capability-gated | Inline path sample produces direct and indirect radiance into high-precision accumulation/history before shared composite/sky. It is a reference/quality path, not yet a performance claim. | `S` | Pending |
| Accumulation invalidation | Implemented path | Scene, camera/view, settings, extent, and relevant lighting state contribute to reference/ReSTIR history validity. Completeness under every editor action requires runtime testing. | `S` | Pending |
| Lighting composite | Implemented path | Direct diffuse, direct specular, direct subsurface, indirect diffuse, indirect specular, and GBuffer emissive are combined before post processing. | `S` | Pending |
| Sky | Implemented path | Dedicated sky lighting and sky motion-vector production are in the graph. Environment/IBL asset breadth is not claimed by this row. | `S` | Pending |

## Ray-Tracing Scene And Shader Tables

| Capability | State | Exact current coverage and limit | Evidence | Release disposition |
| --- | --- | --- | --- | --- |
| Classic TLAS | Capability-gated | Per-view classic TLAS build with an optional refit/update setting; instances carry stable scene identity and shader-table contribution. | `S` | Pending |
| Partitioned TLAS | Capability-gated | Selected through RHI provider capability/preferences with a per-view 3D partition planner. Current build strategy disables instance updates and partition translation and caps operation count at one. | `S` | Pending |
| Shared scene identity | Implemented path | Classic and partitioned acceleration paths derive from the same prepared scene/GPU-scene identity rather than independent material/effect representations. | `S` | Pending |
| Shader-table plan | Implemented path | Logical geometry/material/hit-group classification maps to checked SBT record indices for two ray types: surface and shadow visibility. | `S` | Pending |
| Hit-group coverage | Partial | Opaque and alpha-tested triangle hit groups are authored. Procedural/intersection and callable program coverage was not found in current Renderer effects. | `S` | Pending |
| Plan invalidation | Implemented path | Generation changes follow geometry/hit-group/SBT semantic changes rather than ordinary material-value edits; retired tables/pipelines wait on queue submissions. | `S` | Pending |

## Exposure, Upscaling, Ray Reconstruction, And Output

| Capability | State | Exact current coverage and limit | Evidence | Release disposition |
| --- | --- | --- | --- | --- |
| Manual exposure | Implemented path | Manual exposure, compensation, and bounded exposure values flow through per-view settings and the exposure pass. | `S` | Pending |
| Automatic exposure | Implemented path | Parallel-reduction and downsample-pyramid metering choices, temporal moments/history, target and asymmetric adaptation speeds. | `S` | Pending |
| Async exposure | Implemented path | Exposure can be scheduled as asynchronous compute through the frame graph. Actual overlap is unmeasured. | `S` | Pending |
| Linear upscaler | Implemented path | Built-in spatial linear upscale is the baseline provider/path. Quality and cost need resolution-specific evidence. | `S` | Pending |
| NVIDIA DLSS Super Resolution | Capability-gated | Optional Streamline-backed provider with NativeAA, Quality, Balanced, Performance, and UltraPerformance selections. Provider initialization failure falls back to Linear rather than claiming DLSS output. | `S` | Pending |
| NVIDIA DLSS Ray Reconstruction | Capability-gated | Optional Streamline-backed reconstruction for the ReSTIR lighting route, consuming color, depth, motion, diffuse/specular albedo, normal, roughness, specular hit distance, and exposure. Initialization failure disables the provider path. | `S` | Pending |
| Tone mapping | Implemented path | Reinhard, ACES approximation, and ACES filmic choices. | `S` | Pending |
| Output encoding | Implemented path | Automatic, Linear, and sRGB encoding selections. | `S` | Pending |
| HDR display output | Not found | No PQ, scRGB, HDR10 metadata, display-nit contract, or HDR swapchain mode was found in the current Renderer/RHI output selections. | `S` | Pending |
| Exact debug presentation | Partial | Debug views flow through the existing exposure/tone/output chain. The separate exact display-linear versus scene-referred debug presentation described in the target design is not implemented in this snapshot. | `S` | Pending |

## Debug Views

The public `RenderViewMode` surface contains 16 values:

| Category | Modes | Current boundary |
| --- | --- | --- |
| Final/material | Lit, Wireframe | Wireframe is raster-frontend state; Lit uses the selected lighting/output pipeline. |
| GBuffer | Diffuse/Base Color, Normal, Roughness, Metallic, Emissive, Ambient Occlusion, Subsurface Color, Subsurface Strength | Buffer visualization path; presentation is still subject to current post/output behavior. |
| Lighting | Direct Diffuse, Direct Specular, Direct Subsurface, Indirect Diffuse, Indirect Specular | Visualizes individual lighting targets where the active mode produces them. |
| Scene diagnostics | GPU Scene Instances | Diagnostic visualization of GPU-scene instance data. |

Each selectable mode needs a representative screenshot/capture, expected-value description, backend record, unsupported-combination behavior, and output-transform check before inclusion.

## Diagnostics, Capture, Preview, And Hot Reload

| Capability | State | Exact current coverage and limit | Evidence | Release disposition |
| --- | --- | --- | --- | --- |
| Frame/pass diagnostics | Implemented path | Frame execution, pass execution, scoped GPU timing/events, and frame-graph contract diagnostics are collected through Renderer/RHI diagnostics. | `S` | Pending |
| Mesh diagnostics | Implemented path | Mesh cache/residency/detail snapshots are exposed through the public facade. Correctness and usability are unverified. | `S` | Pending |
| Texture diagnostics | Implemented path | Texture-cache/residency snapshots are exposed through the public facade. | `S` | Pending |
| Memory diagnostics | Implemented path | Renderer memory monitor combines relevant resource/cache/RHI budget information for callers. | `S` | Pending |
| Viewport products | Implemented path | Viewports can target the swapchain or offscreen products used by editor/UI consumers. Product lifetime follows frame retirement. | `S` | Pending |
| Async captures | Implemented path | Requested viewport/final or intermediate products flow through RHI readback and later completion polling. Format/color correctness needs runtime proof. | `S` | Pending |
| Mesh preview | Implemented path | Renderer exposes a mesh-preview product/handle route for editor consumers. | `S` | Pending |
| Shader hot reload | Implemented path | A newly published shader map/library is fully validated and materialized before generation swap; prior pipelines/programs retire after their queue submissions complete. | `S` | Pending |

## Public And Developer Selection Surface

The inspected public settings cover VSync, back-buffer/frame count, adapter preference, tone mapper, exposure, output encoding, upscaler and quality, ray reconstruction, GBuffer frontend, ray-tracing execution, lighting mode, automatic mesh batching, classic TLAS refit, partitioned TLAS activation/planning controls, and view mode. Direct-shadow traversal also has a console-variable surface even where it is not mirrored by the same public settings object.

The settings section persists 27 named Renderer CVars in `/Script/SparkleRenderer.EngineRenderingSettings` inside workspace `Config/DefaultEngine.ini`, replacing that section while retaining other sections. View mode is session state and is not in that persisted set. Adapter preference and back-buffer format are written immediately but reported as pending restart; the other section values are applied to Renderer CVars on commit. File-write failure reporting, concurrent edits, malformed-value diagnostics, and packaged writable-location behavior still require evidence.

Release inventory must be generated from both UI/public settings and console/config surfaces. A hidden but reachable command or CVar is still a selectable capability unless Shipping erases or locks it.

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
- Current debug views do not yet have the target exact display-linear presentation split.
- No current HDR-display output contract was found.

## Primary Source Routes

- Public facade and settings: `Engine/Renderer/Public`.
- Runtime owner and frame sequencing: `Engine/Renderer/Private/Host`, `Engine/Renderer/Private/Concurrency`, and `Engine/Renderer/Private/Frame`.
- Persistent scene/view preparation: `Engine/Renderer/Private/Scene` and `Engine/Renderer/Private/View`.
- Frame graph: `Engine/Renderer/Private/FrameGraph`.
- Geometry/residency: `Engine/Renderer/Private/Meshes`, `Engine/Renderer/Private/Textures`, and `Engine/Renderer/Private/Scene/GpuScene`.
- Raster/ray GBuffer: `Engine/Renderer/Private/Passes/GBuffer`.
- Lighting: `Engine/Renderer/Private/Passes/Lighting` and `Engine/Assets/Shaders/Lighting`, `Engine/Assets/Shaders/BRDF`, and `Engine/Assets/Shaders/Passes/Lighting`.
- Ray scene and execution: `Engine/Renderer/Private/Scene/RayTracing`, `Engine/Renderer/Private/RayTracing`, and `Engine/Renderer/Private/Passes/Lighting/Shadows`.
- Post/output/providers: `Engine/Renderer/Private/Passes/PostProcessing`, `Engine/Renderer/Private/Passes/Presentation`, and `Engine/Renderer/Private/Providers`.
- Diagnostics/editor products: `Engine/Renderer/Private/Diagnostics`, `Engine/Renderer/Private/Editor`, and public Renderer diagnostic/product contracts.
- Build and shader registration membership: `Engine/Renderer/CMakeLists.txt`.
