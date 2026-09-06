# Rendering a Sparkle Frame

Status: feature dossier; current Renderer frame map and local completion contract, not build, runtime, visual, native-validation, performance, or release evidence

Verified: 2026-09-06 against committed `master` revision `8414b5dc` and the live `Engine/Renderer` and RHI service boundaries named below

Responsibility: explain the intent, ownership, data flow, stage order, branches, lifetime, failure boundaries, and tradeoffs of one Sparkle render frame; feature-specific algorithms and limits belong to the linked dossiers

## The Frame In One Sentence

Sparkle accepts one monotonic immutable world submission, updates a persistent render scene, derives one scene-independent view and one frame-local prepared scene, binds them into a dependency-compiled frame graph, produces deferred surface data and ray-traced lighting, converts the scene-linear result into an output product, submits the required GPU queues, and retains every replaced object until its last submission completes.

## Frame At A Glance

```text
GameFramework: immutable RenderFrameSubmission
        |
        v
[0] poll/settle prior capture, residency, and retired generations
        |
[1] accept identity + apply RenderScene delta
        |
[2] resize/topology decision + RHI BeginFrame
        |
[3] upload ready meshes/textures
        |
[4] prepare scene -------> PreparedRenderScene
        |                         |
[5] build view ----------> RenderView + temporal identity
        |                         |
[6] publish GPU scene + prepare BLAS/TLAS/SBT bindings
        |                         |
        +-----------+-------------+
                    v
[7] bind current resources and parameters into FrameGraph
                    |
[8] compile dependencies, barriers, transients, queues, batches
                    |
[9] Scene TLAS -> GBuffer -> Lighting -> Exposure/Reconstruction
                    -> Upscale -> Debug -> Tone map -> Encode -> Copy/Product
                    (no Color Grading, Chromatic Aberration, or Frame Generation)
                    |
[10] record and submit graphics/compute/copy work
                    |
[11] render UI, SubmitFrame, publish tokens, AdvanceFrameInFlight
                    |
[next frame] poll capture/residency and retire completed generations
```

The arrows above describe semantic dependency. Declaration order helps readers, but the frame-graph compiler owns executable ordering, queue assignment, barriers, aliasing, recording chunks, and submission batches from resource-use declarations.

## Stage-by-Stage Frame

`FRAME-*` labels are local reading landmarks for this execution narrative. They are not capability, evidence, pass, or release identifiers; the linked `REN-*` dossiers and `*-E*` plan rows own those identities.

| Stage | Intent and current operation | Input -> owned output | Why it exists | Detail |
| --- | --- | --- | --- | --- |
| `FRAME-00` Submit | GameFramework publishes `RenderFrameSubmission` containing structural scene delta, moved dynamic scene data, view input, and increasing frame ID. `RenderCoordinator` queues or executes it according to serial/threaded configuration. | World-owned data -> immutable renderer request | Renderer never reaches into mutable ECS storage; producer and consumer lifetimes are explicit. | [Scene and View](Features/SceneAndViewPreparation.md) |
| `FRAME-01` Settle | `PollFrameServices` completes capture polling, retired graphs, provider/shader generations, mesh/texture residency, and mesh promotion before touching the next frame. | Prior submission tokens -> reclaimed or newly resident state | GPU completion, not CPU scope exit, decides when resources can be reused or destroyed. | [Diagnostics, Products, and Capture](Features/DiagnosticsProductsAndCapture.md) |
| `FRAME-02` Admit | Reject non-monotonic frame IDs. Apply structural and dynamic scene changes to persistent `RenderScene`; a scene reset unloads scene textures and invalidates view/provider/history state. | Submission scene payload -> new render-scene generation | Persistent resources survive ordinary frames, while explicit reset prevents old scene/history identity leaking into a replacement level. | [Scene and View](Features/SceneAndViewPreparation.md) |
| `FRAME-03` Topology | Resolve output/render extents, output format/target, GBuffer and lighting modes, ray execution plans, provider key, shader generation, and used shader-table generation. Resize or a changed topology retires/rebuilds the graph and invalidates history. | Requested settings + capability reports -> immutable graph configuration | Expensive structural choice is made before recording. Requested and active ray/provider paths can differ only through explicit resolution/failure policy. | [Frame Graph](Features/FrameGraphAndScheduling.md) |
| `FRAME-04` Begin backend | Apply a non-minimized pending swapchain resize, call `RenderDeviceServices::BeginFrame`, begin UI frame state, tick memory diagnostics, and resolve prior timings. | Frame ID + output state -> active frame-in-flight slot and command services | Backend acquisition and frame-slot ownership stay below Renderer policy. | [Frame Graph](Features/FrameGraphAndScheduling.md) |
| `FRAME-05` Upload | Upload ready meshes on the graphics command list and update scene textures through the texture cache. | Ready CPU/cooked resources -> persistent GPU resources | Upload readiness is decoupled from world submission; later scene preparation resolves only usable resources. | [Geometry, Materials, and GBuffer](Features/GeometryMaterialsAndGBuffer.md) |
| `FRAME-06` Prepare scene | Reuse/compile a Tasks graph, resolve primitives and deformation, transform bounds, copy current/previous joint matrices and morph weights, prepare four light kinds, merge results, build RT plan inputs, and commit continuity only on success. | `RenderScene` -> frame-slot `PreparedRenderScene` | Scene-derived work can run in bounded parallel partitions without making the persistent scene a per-view object. | [Scene and View](Features/SceneAndViewPreparation.md) |
| `FRAME-07` Build view | Build camera matrices, frustum, viewport/scissor, display settings, view mode, and temporal uniform from viewport and generation identity. Parallel visibility classifies/culls primitives, then builds raster batches, workload metrics, and a per-view ray-tracing partition plan. | `RenderViewInput` + prepared scene -> frame-slot `RenderView` | Camera, visibility, output, and history are view-owned; the scene remains reusable and is not polluted with viewport policy. | [Scene and View](Features/SceneAndViewPreparation.md) |
| `FRAME-08` Publish GPU scene | Update lighting, geometry, current/previous deformation, hit geometry/material, mesh-instance, descriptor-table, and ray-tracing bindings for the selected frame slot. Configure provider camera/jitter/reset input and prepare ray-tracing frame bindings. | Prepared scene + view -> persistent/frame-indexed GPU bindings | All raster and ray consumers derive from the same scene identity; stable buffers and per-frame slices avoid independent feature copies. | [Geometry and GBuffer](Features/GeometryMaterialsAndGBuffer.md), [Ray Tracing](Features/RayTracing/README.md) |
| `FRAME-09` Bind graph | Bind current TLAS, sky, and GPU-scene buffers; apply defaults plus frame, scene, view, exposure, tone-map, and shadow parameters. Run pass setup and resource-production setup. | Current frame objects -> typed graph parameters and imported resources | The graph shape can persist while per-frame values and native resources change safely. | [Frame Graph](Features/FrameGraphAndScheduling.md) |
| `FRAME-10` Compile and execute | Compile resource versions/dependencies, queue assignment, transient lifetimes/aliasing, barriers, submission batches, and recording plan; materialize transients; record/submit batches; commit texture histories. | Declared graph -> RHI command batches and new histories | Features declare semantic uses; one compiler owns synchronization and lifetime instead of each pass hand-authoring global barriers. | [Frame Graph](Features/FrameGraphAndScheduling.md) |
| `FRAME-11` Produce image | The active graph builds/updates the scene TLAS, writes a raster or ray GBuffer, computes Direct and Indirect surface lighting through the selected ReSTIR/reference mode, composites/sky-fills scene color, meters exposure, optionally reconstructs/upscales, optionally visualizes a debug quantity, tone maps, encodes, and copies to the back buffer or exports a viewport product. There is no Volumetric Lighting, Color Grading, Chromatic Aberration, or Frame Generation stage. | Scene-linear surface/lighting data -> encoded final color | Every implemented feature branch rejoins one presentation boundary, while absent domains remain visible rather than implied. | [Lighting](Features/Lighting/README.md) and [Post Processing](Features/PostProcessing/README.md) |
| `FRAME-12` Submit and retire | Render the UI packet, call `SubmitFrame`, record the graphics token for uploads, advance the frame-in-flight index, and later retire graphs/providers/shaders/resources only when all recorded queue tokens complete. | Recorded GPU work + UI -> presented/product frame and completion state | CPU ownership changes cannot free objects still referenced by any GPU queue. | [Diagnostics, Products, and Capture](Features/DiagnosticsProductsAndCapture.md) |

## What The Graph Declares

`BuildRenderFrameGraph` owns the high-level declaration chain:

```text
Create frame resources
  -> Add ray-tracing scene build
  -> Add GBuffer frontend + sky motion + linear depth
  -> Add lighting producer + composite + sky
  -> Add exposure
  -> Add optional ray reconstruction
  -> Add upscaling when no reconstructed output exists
  -> Add debug visualization
  -> Add tone mapping + output encoding
  -> Copy to back buffer or retain viewport product
```

This is not one fixed list of GPU commands. The topology is specialized before construction.

| Decision axis | Available current branch | Important consequence |
| --- | --- | --- |
| GBuffer | Rasterized; RayTracing | Raster uses vertex/pixel draws and depth attachment. Ray tracing resolves Inline or Pipeline and writes the same semantic outputs with color `R32_Float` device depth. |
| Surface-lighting mode | ReSTIRPathTraced; ReferencePathTraced | Both produce Direct and Indirect surface-lighting lobes and require ray traversal. ReSTIR owns temporal/spatial reuse; reference owns path samples and accumulation. There is no non-ray deferred-lighting branch. |
| Direct lighting | four analytic light kinds; Inline/Pipeline shadow visibility | Produces direct diffuse/specular/subsurface lobes. Shadow traversal resolves independently from GBuffer traversal. |
| Indirect lighting | ReSTIR indirect; reference indirect | Produces indirect diffuse/specular lobes through inline secondary rays. Sky is an environment/background boundary, not broad IBL or atmosphere support. |
| Volumetric lighting | none | No media/fog representation, scattering/transmittance integration, atmosphere/aerial-perspective pass, product, selector, or history enters the frame. |
| Deferred decals | none | No decal data or post-GBuffer composition stage exists. The feature-local target architecture is not current frame behavior. |
| GBuffer/shadow traversal | Automatic; Inline; Pipeline | Automatic resolves capability and readiness before graph construction. Strict modes must not silently become the other traversal frontend. |
| TLAS | Classic; capability/provider-gated partitioned | Both are built from shared prepared-scene identity. Current PTLAS policy remains a narrow subset documented in the ray-tracing dossier. |
| Ray reconstruction | Off; NVIDIA DLSS Ray Reconstruction | Only participates in the ReSTIR lighting route. A successful reconstruction supplies resolved output; otherwise normal upscaling owns resolution conversion. |
| Upscaling | Linear; NVIDIA DLSS Super Resolution | Linear is the baseline. External provider initialization failure resets to Linear rather than claiming DLSS output. |
| Color grading | none | No grading parameters, transform/LUT asset path, pass, shader, selector, or editor workflow enters the frame. Tone-mapper selection is not grading. |
| Chromatic aberration | none | No lens/channel distortion model, pass, selector, or viewport setting enters the frame. |
| Frame generation | none | No generated-frame provider, identity, optical-flow input, pacing, UI policy, or extra presentation enters the frame. Reflex/PCL latency coordination is not synthesis. |
| Presentation target | BackBuffer; ViewportProduct | Back-buffer frames add the copy into the imported presentable resource. Offscreen/editor viewports publish named products without owning swapchain presentation. |
| Debug view | Lit, Wireframe, GBuffer, lighting, GPU-scene modes | Visualization can replace resolved scene color before the common tone-map/encode path; exact display-linear debug presentation is not implemented. |

## Principal Resource Flow

| Product | Current format/shape | Produced by | Consumed by or exported as |
| --- | --- | --- | --- |
| Scene color | `R16G16B16A16_Float`, render extent | ReSTIR composite/sky or reference accumulation | upscaling, debug, tone mapping |
| Scene depth | `R32_Float`, render extent | device-depth linearization | lighting, sky/background, viewport depth, provider inputs |
| GBuffer base color | `R8G8B8A8_UNorm` | raster or ray GBuffer | lighting, debug |
| GBuffer normal | `R16G16B16A16_Float` | raster or ray GBuffer | lighting, reconstruction, debug, viewport normal product |
| GBuffer material | `R8G8B8A8_UNorm` | raster or ray GBuffer | metallic/roughness/AO/F0 lighting terms and debug |
| Emissive | `R16G16B16A16_Float` | raster or ray GBuffer | lighting composite and debug |
| Subsurface | `R8G8B8A8_UNorm` | raster or ray GBuffer | direct subsurface/composite and debug |
| Motion vector | `R16G16_Float` | GBuffer plus sky-motion handling | temporal reuse, accumulation, reconstruction/upscaling |
| Lighting lobes | ReSTIR uses `R16G16B16A16_Float`; reference uses `R32G32B32A32_Float` | selected lighting producer | composite, debug, reference sample, reconstruction |
| Exposure | 1x1 `R32G32B32A32_Float` | manual/automatic exposure pass | reconstruction/upscaling and tone mapping |
| Resolved scene color | `R16G16B16A16_Float`, output extent | ray reconstruction or upscaling | debug and tone mapping |
| Encoded scene color | linear counterpart of output format, output extent | output-encoding compute pass | back-buffer copy and `FinalSceneColor` viewport product |

The table states the current internal contract, not precision adequacy or backend format support. Those require `REN-E03`, `REN-E04`, `REN-E17`, and `RHI-E04` evidence.

## Identity, History, And Invalidation

One frame carries several identities because they answer different lifetime questions:

| Identity | Changes when | What it protects |
| --- | --- | --- |
| Frame ID | a newer submission is admitted | ordering and per-frame diagnostics |
| Scene generation | the persistent render scene is reset/replaced | scene resources and temporal continuity |
| Shader generation | a fully validated shader map/library becomes active | pipelines/programs and graph materialization |
| Image-provider generation/key | provider selection or active provider generation changes | DLSS/RR resources and stale provider/graph pairing |
| Graph topology generation | graph structure is rebuilt | temporal state tied to pass/resource topology |
| Shader-table-plan generation | geometry/hit-group/SBT semantics change | native ray record indexing; only matters to graphs that use the scene table |
| Frame-in-flight index | RHI advances the reusable slot | frame-local prepared scene/view and dynamic GPU storage |

Resize, scene reset, topology/provider/shader changes, and relevant table-plan changes invalidate affected history before reuse. Old graphs, frame slots, providers, and shader generations enter retirement queues carrying the last submission state across all used queues.

## Design Decisions And Tradeoffs

| Decision | Intent | Tradeoff and current risk |
| --- | --- | --- |
| Immutable world-to-render submission | Make ownership, threading, and frame identity inspectable. | Requires deliberate data extraction/copy budgeting and complete deltas; stale or missing publication is not repaired by Renderer querying the world. |
| Persistent scene, frame-local prepared scene, view-owned view | Keep durable resources separate from per-frame computation and per-viewport policy. | Preparation and GPU publication must keep generations/history coherent across edits, reloads, and multiple views. |
| Declarative frame graph | Centralize dependencies, barriers, aliasing, queues, recording, and diagnostics. | Compile/setup currently occurs per executed frame even when the graph object persists; CPU cost is unmeasured and should not be optimized speculatively. |
| Topology chosen before graph construction | Remove per-pass ambiguity and make unavailable strict paths fail early. | A setting/provider/shader/table change rebuilds and retires graph generations, so churn and failure recovery matter. |
| Shared semantic GBuffer and shadow contracts over two RT frontends | Keep traversal mechanism below feature meaning. | Inline/native parity, SBT mapping, backend behavior, and performance crossover need independent proof. |
| Deferred separated material and lighting products | Enable common shading, debug inspection, and reconstruction guides. | Memory/bandwidth cost is substantial; transparent/transmission and broader PBR lobes are outside the current contract. |
| Completion-driven retirement | Prevent use-after-free across asynchronous queues. | Retired generations can accumulate when completion stalls; boundedness needs stress evidence. |
| One common presentation path | Make exposure, scale, debug, tone, encoding, capture, and output ownership explicit. | Current debug quantities are still affected by presentation semantics, and HDR display output is absent. |

## Failure And Safe-State Boundaries

- A non-monotonic submission is ignored before it changes the active frame ID.
- Failed scene preparation clears frame-local prepared output and resets continuity; it does not publish partial prepared data.
- Invalid GBuffer or lighting enum values fail graph construction rather than selecting an arbitrary mode.
- Incomplete TLAS or hit/material bindings are fatal graph-execution contract violations.
- Strict ray execution requires its selected frontend; Automatic may choose a documented supported alternate.
- Provider initialization failure shuts down that provider and resets selection to Linear or Off.
- A replacement shader generation is not activated until complete runtime materialization succeeds; the previous generation remains active/retired by queue state.
- Minimized or invalid-size windows do not perform a swapchain resize; history is invalidated around a real topology resize.

These source-level policies become the stable feature contract below. The [Capability Evidence Plan](../../../../Plans/CapabilityEvidence.md#renderer-evidence) selects the smallest candidate checks; it does not redefine the pass conditions.

## Acceptance Criteria

- `AC-FRM-01` — monotonic immutable submissions produce the same accepted frame IDs, scene/view state, graph plan, products, and terminal result with serial and threaded Renderer coordination.
- `AC-FRM-02` — every admitted frame follows the documented stage order and every listed product has exactly one producer, declared format/extent/identity, and only its documented consumers.
- `AC-FRM-03` — scene data remains persistent and scene-owned, view state remains view-owned, frame preparation remains frame-local, and no Renderer stage queries mutable world/editor state after submission.
- `AC-FRM-04` — feature, provider, traversal, and topology choices resolve before graph construction; invalid or unavailable strict choices fail explicitly and Automatic exposes its selected supported route.
- `AC-FRM-05` — scene, view, extent, shader, provider, graph-topology, and shader-table identities invalidate only their affected histories and never mix data from incompatible generations.
- `AC-FRM-06` — graph execution submits the declared queue batches once, advances the reusable frame slot only under its completion contract, and retires every old graph/resource/provider/shader generation after all recorded queue tokens complete.
- `AC-FRM-07` — the final encoded result reaches exactly the selected swapchain or viewport product with matching frame/view/extent/format metadata; minimized or invalid-size windows never publish a fabricated frame.
- `AC-FRM-08` — shutdown, scene reset, resize, provider failure, failed preparation, failed replacement, and delayed GPU completion settle owned work within declared bounds without partial publication, use-after-free, stale history, or unbounded retirement growth.

## Controlled Failure Modes And Checks

| Failure ID | Injection and safe state | Detecting check |
| --- | --- | --- |
| `FM-FRM-01` | repeat or regress a submission ID | request is ignored before active frame or scene/view state changes | `CHK-FRM-01` |
| `FM-FRM-02` | fail scene/view preparation or provide an invalid GBuffer/lighting/topology choice | no partial prepared state or arbitrary mode reaches graph execution; exact failure is observable | `CHK-FRM-01`, `CHK-FRM-02` |
| `FM-FRM-03` | remove strict traversal/provider/program/SBT capability or fail provider/replacement initialization | strict request fails before graph construction; documented Automatic fallback or prior valid generation remains inspectable | `CHK-FRM-02` |
| `FM-FRM-04` | resize, reset scene, switch provider/shader/table plan, or reuse a stale viewport/product while prior work is in flight | incompatible history/product is rejected or reset and generations remain isolated | `CHK-FRM-03` |
| `FM-FRM-05` | delay graphics/compute/copy completion while churning frames, graphs, providers, shaders, resize, and shutdown | resources remain alive through last use, retirement stays bounded by declared policy, and shutdown drains exactly once | `CHK-FRM-04` |

| Check | Exercise and oracle | Covers |
| --- | --- | --- |
| `CHK-FRM-01` | replay identical immutable submissions through serial/threaded coordination, duplicate/regressed IDs, and failed preparation; compare accepted IDs, prepared state, graph/product manifests, and terminal result | `AC-FRM-01`–`AC-FRM-03`; `FM-FRM-01`, `FM-FRM-02` |
| `CHK-FRM-02` | enumerate/force valid, invalid, Automatic, and unavailable feature/provider/traversal/topology cells; inspect pre-graph plan and requested/active diagnostics | `AC-FRM-02`, `AC-FRM-04`; `FM-FRM-02`, `FM-FRM-03` |
| `CHK-FRM-03` | dual-view sequence across camera cut, scene reset, resize/minimize/restore, provider/shader/table changes, and stale product injection; compare identity, history resets, and final metadata | `AC-FRM-05`, `AC-FRM-07`; `FM-FRM-04` |
| `CHK-FRM-04` | paired-backend run with delayed queue completion, frame-slot pressure, generation churn, device/provider failure, and shutdown; assert submissions, queue tokens, retained-generation high-water, reclamation, and native validation | `AC-FRM-06`, `AC-FRM-08`; `FM-FRM-03`, `FM-FRM-05` |

This contract is **defined but unproved**. Completion requires every `AC-FRM-*` to pass, every applicable `FM-FRM-*` to be deliberately exercised through its named `CHK-FRM-*`, all affected child feature contracts to pass, and the candidate report to retain exact revision/configuration/backend/evidence and limitations.

## Where To Go Deeper

| Concern | Owning detail |
| --- | --- |
| CPU scene/view preparation and GPU-scene publication | [Scene and View Preparation](Features/SceneAndViewPreparation.md) |
| Static/instanced/skinned/morphed geometry, material roles, raster/ray GBuffer | [Geometry, Materials, and GBuffer](Features/GeometryMaterialsAndGBuffer.md) |
| Frame graph, barriers, queues, transients, recording, submission, retirement | [Frame Graph and Scheduling](Features/FrameGraphAndScheduling.md) |
| Shared lighting-mode and composite boundary | [Lighting](Features/Lighting/README.md) |
| Analytic lights, direct BRDF lobes, reservoirs, and shadow visibility | [Direct Lighting](Features/Lighting/DirectLighting.md) |
| ReSTIR/reference secondary transport, histories, environment, and sky | [Indirect Lighting](Features/Lighting/IndirectLighting.md) |
| Explicit absence of media, fog, atmosphere, and aerial perspective | [Volumetric Lighting](Features/Lighting/VolumetricLighting.md) |
| BLAS/TLAS, inline/native execution and shader-table identity | [Ray Tracing](Features/RayTracing/README.md) and [execution architecture](Features/RayTracing/ExecutionArchitecture.md) |
| Post-processing order and shared invariants | [Post Processing](Features/PostProcessing/README.md) |
| Exposure and adaptation | [Exposure](Features/PostProcessing/Exposure.md) |
| Image reconstruction, upscaling, and provider lifetime | [Image Reconstruction and Upscaling](Features/PostProcessing/ImageReconstructionAndUpscaling.md) |
| Tone mapping | [Tone Mapping](Features/PostProcessing/ToneMapping.md) |
| Explicit absence of color grading | [Color Grading](Features/PostProcessing/ColorGrading.md) |
| Explicit absence of chromatic aberration | [Chromatic Aberration](Features/PostProcessing/ChromaticAberration.md) |
| Explicit absence of frame generation | [Frame Generation](Features/PostProcessing/FrameGeneration.md) |
| Debug handoff, encoding, and output targets | [Presentation and Output](Features/PostProcessing/PresentationAndOutput.md) and [Debug Views](Features/DebugViews/README.md) |
| Host UI and editor viewport composition | [UI and Viewport Composition](Features/UiAndViewportComposition.md) |
| Timing, memory, product publication and asynchronous capture | [Diagnostics, Products, and Capture](Features/DiagnosticsProductsAndCapture.md) |
| Every exact feature row and known absence | [Renderer Capability Inventory](CapabilityInventory.md) |
| Cross-module/backend comparison | [Graphics Feature Coverage Matrix](../../../CrossModule/GraphicsCoverageMatrix.md) |
| End-to-end GameFramework/ShaderCompiler/RHI handoffs | [Graphics Feature Execution Traces](../../../CrossModule/FeatureExecutionTraces.md) |

## Primary Source Route

The shortest code path is [`FramePipeline::OnRender`](../../../../../Engine/Renderer/Private/Frame/FramePipeline.cpp) -> [`FramePipeline::PrepareRenderFrame`](../../../../../Engine/Renderer/Private/Frame/FramePipeline.cpp) -> [`BuildRenderFrameGraph`](../../../../../Engine/Renderer/Private/Frame/Graph/BuildRenderFrameGraph.cpp) -> [`RenderFrameGraphExecution::Execute`](../../../../../Engine/Renderer/Private/Frame/Graph/ExecuteRenderFrameGraph.cpp) -> [`FrameGraph::Compile`](../../../../../Engine/Renderer/Private/FrameGraph/FrameGraph.cpp) and [`FrameGraph::Execute`](../../../../../Engine/Renderer/Private/FrameGraph/Execution/FrameGraphExecution.cpp) -> RHI submission services.
