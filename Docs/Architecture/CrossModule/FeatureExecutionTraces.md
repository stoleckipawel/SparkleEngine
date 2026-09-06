# Graphics Feature Execution Traces

Status: capability snapshot; vertical producer-to-consumer traces; not runtime proof or release approval

Snapshot: 2026-09-06 at committed `master` revision `8414b5dc`; current source and build membership inspected; evidence `S` only

Scope: ownership, selection, data production, frame-graph scheduling, shader/RHI consumption, output, history, retirement, and failure behavior for the principal RHI/Renderer/shader-delivery paths

Owners: the concrete producer and consumer modules named in each trace; this document maps those boundaries and does not transfer them

Horizontal companion: [Graphics Feature Coverage Matrix](GraphicsCoverageMatrix.md)

Module inventories: [RHI](../Modules/Engine/RHI/CapabilityInventory.md), [Renderer](../Modules/Engine/Renderer/CapabilityInventory.md), and [Shader Compilation](../Modules/Tools/ShaderCompiler/README.md)

Renderer-local frame intent, stage order, resource flow, and design tradeoffs: [Rendering a Sparkle Frame](../Modules/Engine/Renderer/RenderingASparkleFrame.md). This cross-module document owns only the handoffs among GameFramework, Renderer, ShaderCompiler, and RHI.

## Common Frame Spine

Every render mode enters the same frame owner. This is the shortest trace for locating a stale scene, view, graph, provider, or GPU-lifetime defect.

| Stage | Current owner and operation | Published boundary | Failure/lifetime behavior |
| --- | --- | --- | --- |
| 1. Submission | Gameplay/Application supplies `RenderFrameSubmission`, `RenderViewInput`, timing, and optional UI packet to Renderer | Immutable/movable submission crosses into `RenderCoordinator` | Serial and threaded execution are separate coordinator modes; parity is unexecuted |
| 2. Frame admission | `FramePipeline::BeginFrame` polls services, accepts the monotonic submission and scene delta, applies pending resize/topology changes, then begins the backend frame | Current frame ID and in-flight slot | A scene reset unloads scene textures and invalidates view/history |
| 3. Persistent scene update | `RenderScene` applies submission deltas; `GpuMeshCache` uploads ready meshes; `TextureCache` reconciles scene textures | Scene-owned persistent CPU/GPU identity | Missing persistent GPU publication is fatal at frame-graph binding |
| 4. Scene preparation | `RenderScenePreparation::Execute` creates frame-local `PreparedRenderScene` from the persistent scene | Prepared scene references/counts/derived lighting data | Prepared data is frame-owned; it does not become a second persistent scene authority |
| 5. View preparation | `RenderViewBuilder` and `RenderViewPreparation` derive camera, temporal, display, ray-plan, and viewport data in `RenderView`/`RenderViewState` | View-owned uniforms and history validity | Camera/topology/scene invalidation resets temporal validity rather than mutating scene ownership |
| 6. GPU-scene publication | Persistent scene updates lighting, geometry, deformation, and ray-hit buffers for the selected in-flight frame | `RenderSceneGpuBindings` contains resource/size/stride bindings | Frame graph requires all declared buffers, including empty-safe publications |
| 7. Ray preparation | `RenderRayTracingScene::PrepareRayTracingFrame` consumes prepared scene plus resolved effect plans | BLAS/TLAS build data and ray frame bindings | Capability-incompatible strict frontend resolves to `None`; no silent raster shadow/lighting substitute |
| 8. Graph execution | Compiled `FrameGraph` binds persistent buffers/history and executes typed raster, compute, ray-tracing, transfer, and external-provider passes | Queue submissions and viewport products | Topology key changes rebuild execution and invalidate history |
| 9. Submission/presentation | `RenderDeviceServices::SubmitFrame` submits queues; UI overlay and present complete the frame | Per-queue `RhiSubmissionToken` and presented/backed viewport product | Resources/providers/pipelines retain last-use state until queue completion |
| 10. Retirement | Frame execution, provider generations, shader/pipeline generations, capture readbacks, texture residency, and mesh uploads are polled | Completed generations become destructible | Responsiveness alone does not prove correct retirement or absence of growth |

The source ownership rule is concrete: `RenderScene` owns persistent scene data, `PreparedRenderScene` is a frame-local derived view of it, `RenderView` owns camera/temporal/view data, and `FramePipeline` sequences publication and execution.

## Trace 1: Raster GBuffer To Presented Pixel

| Step | Producer -> consumer | Exact data/operation | Gate or boundary |
| --- | --- | --- | --- |
| Material ingestion | Cooked/runtime material -> `RenderScene` material records | Factors plus eight texture-role identities, alpha mode/cutoff, sidedness, and shading data | General transparent blend/transmission has no complete render path |
| Texture residency | `TextureCache` -> material descriptor table | Resident SRVs are written into the material’s bindful table | Eight roles; this path is not the 4096-entry ray material table |
| Mesh preparation | Scene mesh instances -> `GpuMeshCache`/batch builder | Vertex/index streams, transforms, prior transforms, skin/morph data, and compatible auto batches | Public vertex contract is position/UV/normal/tangent and triangle list |
| Graph declaration | GBuffer target factory -> frame graph | Six color-like attachments plus depth: base, normal, material, emissive, subsurface, motion, `D32_Float` depth | Required format attachment support is adapter-queried but unexecuted |
| Pipeline materialization | `GBufferMeshPass` -> runtime pass cache -> RHI | Typed `GBufferVS/GBufferPS`, binding layout, graphics PSO and material descriptors | Shader generation and material/pipeline keys control reuse |
| Draw | Batch drawer -> graphics command list | Indexed triangle draws write the GBuffer | Alpha mask can discard; no blend pass follows |
| Derived buffers | `SkyMotionVectorCS` and `SceneDepthCS` | Completes background motion and writes linear `R32_Float` scene depth | Runs after either raster or ray GBuffer frontend |
| Lighting | Selected lighting producers -> composite -> sky | Reads common GBuffer semantics and produces HDR `SceneColor` | Raster GBuffer does not imply non-ray lighting: both current lighting modes still trace rays |
| Post Processing | exposure -> optional reconstruction/upscale -> debug -> tone map -> encode -> copy | Output extent/color format becomes back buffer or viewport product | Current debug views still pass through presentation semantics; color grading, chromatic aberration, and frame generation are absent |

Vertical completeness risk: importer/cooker fidelity for every material role remains a separate asset-pipeline audit. This trace proves the Renderer-side consumer path exists, not that every source format populates it correctly.

## Trace 2: Ray GBuffer, Inline And Native Pipeline

| Step | Shared semantic contract | Inline adapter | Native-pipeline adapter |
| --- | --- | --- | --- |
| Selection | `GBufferAlgorithm::RayTracing` and capability report resolve an immutable execution plan | Strict Inline or Automatic fallback when pipeline is unavailable | Strict Pipeline or Automatic preference when ready |
| Required capabilities | AS and descriptor indexing are common | Inline ray query | RT pipeline plus shader-table readiness |
| Scene geometry | `RenderRayTracingScene` owns BLAS/TLAS preparation from scene meshes/instances | Same scene TLAS/hit buffers | Same scene TLAS/hit buffers and scene shader-table plan |
| Material binding | `RayTracingHitMaterial` indices address the fixed 4096-entry material texture table | Bound to compute parameter layout | Bound as ray-generation global parameters; hit stages use scene record identity |
| Programs | Shared hit/material shader includes define semantics | `RayTracingGBufferInlineCS` | RGS, Miss, ClosestHit, AnyHit |
| Alpha mask | Hit evaluation rejects masked texels | Inline candidate handling | Any-hit program |
| Deformation | Current/previous joint matrices and morph weights are available for shading/motion; ray geometry preparation owns deformed positions | Same | Same |
| Output | Both adapters write identical seven GBuffer semantics, with `R32_Float` DeviceZ | Compute dispatch | Trace-rays dispatch |
| Failure | Plan carries exact unavailable/invalid reason; graph cannot advertise a working frontend when `Active=None` | No automatic substitution to raster | Pipeline/shader-table creation throws on incomplete readiness or invalid limits |

The native shader-table plan has exactly two current ray types, `Surface` and `ShadowVisibility`, and two hit-group semantics, opaque and alpha-tested. Its checked record index is `rayContribution + (2 * geometryIndex) + instanceContribution`; changes to ray-type layout, geometry layout, or alpha-test hit-group semantics invalidate the plan, while ordinary material-value changes do not redefine logical SBT indexing. This relationship needs executable index/capture evidence before release.

## Trace 3: ReSTIR Real-Time Path

| Step | Operation | Inputs -> outputs | Temporal/fallback boundary |
| --- | --- | --- | --- |
| Direct temporal | Reprojects/selects direct-light reservoir candidates | GBuffer, motion, light buffers, previous direct sample/weight/surface -> current temporal sample/weight | History hash and view validity decide whether previous data is valid |
| Direct spatial | Reuses neighboring candidates | Temporal sample/weight plus current surface -> spatial result/history surface | Spatial quality/bias unmeasured |
| Visibility | Traces selected-light visibility | TLAS, hit geometry/material buffers, 4096 texture table -> RGBA32F shadow visibility | Inline or native pipeline via independent shadow execution plan; no shadow-map fallback |
| Direct resolve | Evaluates visible direct sample | Reservoir/visibility + four light-type buffers + material GBuffer -> direct diffuse/specular/subsurface | Directional, point, spot, rect capacities and overflow policy need runtime evidence |
| Indirect temporal | Reprojects and traces indirect candidate data | Prior indirect reservoir, GBuffer, TLAS, hit/material/light/sky data -> temporal reservoir | Inline ray query only |
| Indirect spatial | Reuses neighboring indirect candidates | Temporal reservoir and current surface -> spatial reservoir | Inline ray query only |
| Indirect resolve | Evaluates selected indirect sample | Reservoir + TLAS/material/sky -> indirect diffuse/specular and RR guides | Inline ray query only; guides are full extent only when RR is enabled |
| Composite | Combines five lighting lobes; sky fills background | Lighting targets + GBuffer emissive/subsurface/depth -> RGBA16F scene color | RR is optional post-lighting reconstruction, not the lighting producer |

ReSTIR history invalidates when the prepared-scene invalidation hash changes, when temporal view validity is zero, or when the graph topology changes. The same invalidation also resets image-provider history.

The [Direct Lighting dossier](../Modules/Engine/Renderer/Features/Lighting/DirectLighting.md) owns the direct reservoir/visibility/BRDF result. The [Indirect Lighting dossier](../Modules/Engine/Renderer/Features/Lighting/IndirectLighting.md) owns secondary transport and its histories. No Volumetric Lighting stage participates in this trace; its [negative capability dossier](../Modules/Engine/Renderer/Features/Lighting/VolumetricLighting.md) records the missing media/atmosphere ownership.

## Trace 4: Convergent Reference Path

| Step | Operation | Exact contract | Boundary |
| --- | --- | --- | --- |
| Mode selection | `LightingMode::ReferencePathTraced` | Graph allocates all five lighting lobes as RGBA32F | Still depends on inline ray queries; “reference” does not mean CPU or native-pipeline traversal |
| Direct sample | `PathTracedDirectLightingCS` | GBuffer, four light buffers, TLAS, hit/material buffers, fixed texture table -> direct diffuse/specular/subsurface | Inline only |
| Indirect sample | `PathTracedIndirectLightingCS` | GBuffer, sky, TLAS, deformation/hit/material buffers, fixed texture table -> indirect diffuse/specular | Inline only; bounce-control behavior unexecuted |
| Sample validity | Reference sample descriptor carries sample color/validity for accumulation | Current lighting sample -> validity-aware accumulation inputs | Exact invalid sample behavior needs numeric test |
| Accumulation | `ReferenceLightingAccumulationCS` | Current sample + previous RGBA32F history + motion + validity -> current history and scene sample | Prepared-scene/view invalidation hash resets history |
| Composite/present | Common composite, sky, exposure, upscale, debug, presentation | High-precision lobes eventually become RGBA16F scene/output intermediates | It is a convergence/reference feature, not yet an accepted correctness oracle; see [`PTD-00`](../Modules/Engine/Renderer/Features/Lighting/OfflinePathTracer/Discovery.md) |

## Trace 5: External Image Provider Lifecycle

| Step | Owner and operation | Contract/failure behavior |
| --- | --- | --- |
| Selection | CVars/settings choose Linear or NVIDIA DLSS, and Off or NVIDIA RR | Provider choice participates in the frame-graph topology key |
| Construction | Factories return engine linear/no-op choices or NVIDIA Streamline-backed providers | NVIDIA source is isolated in `SparkleRendererNvidiaStreamlineProviders`; compile definition records SDK availability |
| Capability handoff | `RendererImageProviderStack` passes `RhiCapabilities` and explicit device/queue interop | D3D12 may evaluate when native device/queue/command list exist; Vulkan explicitly refuses external evaluation |
| Initialization failure | Stack shuts down the failed provider | Upscaler selection is reset to Linear; RR is reset Off; warning is logged |
| Per-frame setup | Camera, jitter/temporal identity, extents, reset-history flag, and provider generation are supplied | Provider generation prevents stale graph/provider pairing |
| Graph evaluation | External pass receives tagged color/depth/motion/exposure and, for RR, lighting/reconstruction-guide inputs | Engine creates a valid resolved-color target around the provider path |
| Reconfiguration | Old providers move to a retired generation with last-use tokens; new stack initializes and generation increments | Destruction waits until every relevant queue token completes |

## Trace 6: Post Processing To Published Output

| Step | Owner and operation | Input -> output | Boundary |
| --- | --- | --- | --- |
| Exposure | manual or automatic metering/adaptation | scene-linear `SceneColor` + per-view settings/history -> 1x1 exposure | Runs before debug replacement; two metering methods and optional async compute are source-present but unproved |
| Ray Reconstruction | optional NVIDIA DLSS RR for the ReSTIR topology | scene/depth/motion/lobes/guides/exposure -> output-extent `ResolvedSceneColor` | D3D12/Streamline/capability gated; unavailable provider resolves Off |
| Upscaling | Linear or NVIDIA DLSS SR when reconstruction did not produce resolved color | render-extent scene/depth/motion/exposure -> output-extent `ResolvedSceneColor` | Exactly one resolution owner; NVIDIA failure resets to Linear |
| Debug handoff | selected visualization may replace resolved scene color | requested GBuffer/lighting/scene product -> visualization color | Current diagnostics still flow through tone mapping and encoding |
| Tone mapping | exposure plus Reinhard, ACES approximation, or ACES fitted filmic | resolved scene-referred HDR -> display-linear `ToneMappedSceneColor` | No public None/bypass; fixed operators are not a color-grading system |
| Output | Automatic/Linear/sRGB encoding, then copy or product publication | display-linear color -> encoded back buffer or `FinalSceneColor` | No PQ/scRGB/HDR10/display-nit contract |
| Explicit absent stages | no graph owner | no color-grading/LUT transform, chromatic-aberration lens effect, or generated-frame synthesis occurs | Reflex/PCL latency coordination and temporal reconstruction are not frame generation |

The [Post Processing family dossier](../Modules/Engine/Renderer/Features/PostProcessing/README.md) owns this order. Its child dossiers keep each supported or absent capability independently reviewable.

## Trace 7: Shader Authoring To Runtime Generation

| Step | Owner | Exact transition | Failure boundary |
| --- | --- | --- | --- |
| Registration | Renderer contract target | Typed class -> virtual source, entry, stage, feature flags, ray metadata, parameter metadata | Duplicate/empty/inconsistent registration is rejected |
| Source resolution | ShaderCompiler | `/Engine` and `/Project` virtual roots -> canonical file/include closure | Mount escape, missing include, or invalid virtual path fails the job |
| Planning | ShaderCompiler cook planner | All, one shader ID, or reverse-dependency closure from changed paths -> immutable jobs | Removed registrations are not preserved; unaffected valid entries are preserved in incremental cook |
| Compilation | DXC or Slang backend | Source + target + policy + binding remaps -> binary, reflection, diagnostics, hashes | Current runtime targets are DXIL SM6.6 and SPIR-V 1.6; Slang stage mapping is vertex/pixel/compute only |
| ABI validation | Shared validators | Reflected resources/constants/stage/ray contract compared with typed C++ metadata | Mismatch prevents publication |
| Staging | Publisher | Map, library, dependency data, and recook signal are written as a candidate file set | Staged map/library must open together and share publication identity |
| Commit | Atomic publication | Candidate file set replaces current products | Cancellation/failure cleanup preserves prior valid generation by design; executable fault tests remain open |
| Reload admission | Application recook coordinator | Fresh publication ID, active-authority paths, and file hashes are checked before Renderer reload | Missing, partial, outside-authority, stale, or hash-mismatched publication is rejected before active shaders change |
| Materialization | Renderer runtime pass cache | Complete registration set for the selected backend target -> RHI programs, layouts, graphics/compute/ray pipelines and shader tables | Missing target/registration/signature/composition prevents generation publication |
| Swap/retire | Renderer | Whole validated generation becomes active; previous generation records queue last-use | Old pipelines/programs retire only after GPU completion |

The exact 35-program registration set is listed in [Shader Program Catalog](../Modules/Engine/Renderer/Features/ShaderPrograms.md).

## Trace 8: Viewport Product Capture

| Step | Producer -> consumer | Exact behavior |
| --- | --- | --- |
| Request | Public Renderer facade -> `RenderCoordinator` | Allocates capture ID; serial path begins immediately, threaded path sends a control command |
| Product resolution | `ViewportCaptureService` -> published viewport products/frame graph | Resolves requested final color, depth, or normal product together with frame, scene, shader, and provider generation identity |
| RHI request | Capture service -> backend `RhiCaptureService` | Starts asynchronous texture readback when resource/format/request is acceptable |
| Poll | `FramePipeline::PollFrameServices` | Polls capture completion alongside retirement and residency services |
| Publication | Pipeline -> coordinator read state | Completed captures are moved to producer-facing queue; coordinator retains at most three completed captures and drops the oldest beyond that |
| Take | Caller -> `TryTakeViewportCapture` | Moves one completed result out; no response means pending or absent, not success |

## Trace Closure Rule

A vertical path is release-complete only when every stage has an owned producer, an owned consumer, a defined failure/fallback, and candidate-bound executable evidence. Source closure in this document earns only `S`; the corresponding `B`, `R`, `N`, `P`, and `A` work remains in the [Capability Evidence Plan](../../Plans/CapabilityEvidence.md).
