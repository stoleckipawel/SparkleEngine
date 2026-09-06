# Renderer Feature Dossiers

Status: Renderer feature index; routes current source claims and design intent, not release approval

Scope: give each independently meaningful Renderer feature family an explicit definition and owning document, while the parent inventory retains the exact row ledger

Authority boundary: these dossiers own how Renderer features work, why their boundaries exist, how their stages interact, and what feature-local criteria, controlled failures, checks, and completion definition apply. [Capability Inventory](../CapabilityInventory.md) owns the compact implementation-state/evidence ledger; [Feature Selector Catalog](RuntimeConfiguration/FeatureSelectorCatalog.md) owns exact reachability; [Acceptance](../../../../../Acceptance/README.md) owns cross-feature reports, workload/release gates, and actual candidate verdicts. Context repeated in a dossier explains its feature and does not independently promote the corresponding row.

## Dossier Contract

A Renderer feature dossier must answer—or explicitly mark unanswered—what result the feature produces; how it is selected; requested versus active behavior; inputs and outputs; scene/view/frame ownership; pass and shader stages; RHI/backend prerequisites; history and retirement; limits and unsupported combinations; failure/recovery/diagnostics; design intent and tradeoffs; exact source route; capability IDs; acceptance criteria; controlled failure modes; checks; definition of done; and release-report destination. Unknowns remain named gaps rather than implied support.

## Why This Depth And Granularity

- A current-state narrative lets an engineer find the real owner and extension seam without reverse-engineering every pass, while source/build membership remains the implementation authority.
- Vertical traces expose handoff defects in identity, lifetime, publication, failure, and retirement that a flat feature list cannot reveal.
- Horizontal matrices prevent one backend, traversal mode, material/geometry case, viewport, lifecycle state, or provider fallback from lending an unjustified verdict to unlike cells.
- Stable `AC-*`, `FM-*`, and `CHK-*` identities make later evidence falsifiable and reusable without storing candidate results in Architecture.
- Explicit negative dossiers prevent familiar vocabulary, nearby SDK support, dormant code, target designs, or artifacts from being advertised as implemented features.
- Feature-sized ownership keeps the documents maintainable: shared orchestration is described once at the family/frame boundary, while independently selectable or independently failing results own their own contract.

## Folder Map

| Feature family | Why it is grouped here |
| --- | --- |
| [Frame Execution](FrameExecution/README.md) | frame graph, temporal continuity, and latency coordination share frame identity and ordering |
| [Geometry And Resources](GeometryAndResources/README.md) | residency, visibility/draw preparation, and surface/GBuffer production form the resource-to-surface route |
| [Runtime Configuration](RuntimeConfiguration/README.md) | settings state/persistence and exact selector reachability are separate views of requested and active configuration |
| [Shader Runtime](ShaderRuntime/README.md) | exact program membership feeds pipeline materialization, typed binding, generation, and retirement |
| [Viewport And Diagnostics](ViewportAndDiagnostics/README.md) | products, capture, observations, UI packets, and viewport publication meet at the host-facing boundary |
| [Scene And View Preparation](SceneAndViewPreparation/README.md) | persistent scene, per-frame scene/view derivation, GPU-scene publication, and their integration acceptance |
| [Lighting](Lighting/README.md) | shared lighting composition routes direct, indirect, volumetric, and offline-reference capabilities |
| [Ray Tracing](RayTracing/README.md) | current acceleration/traversal behavior and target execution architecture share one feature family |
| [Post Processing](PostProcessing/README.md) | ordered reconstruction and display families transform scene color into the published image |
| [Debug Views](DebugViews/README.md) | current diagnostic modes, view/show control, presentation architecture, and acceptance share one feature identity |
| [Deferred Decals](DeferredDecals/README.md) | current absence, target composition architecture, and acceptance remain one feature family |

## Feature Map

| Capability families | Feature dossier | Primary result |
| --- | --- | --- |
| `REN-OWN-*` | [Rendering a Sparkle Frame](../RenderingASparkleFrame.md) | accepted submission becomes one submitted/presented frame with explicit ownership and identity |
| `REN-FG-*` | [Frame Graph and Scheduling](FrameExecution/FrameGraphAndScheduling.md) | typed feature declarations become synchronized GPU work and completion-owned lifetime |
| `REN-PIPE-*`, retained `REN-DIAG-08` | [Pipeline Materialization and Typed Binding](ShaderRuntime/PipelineMaterializationAndTypedBinding.md) | registered/cooked shader contracts become checked pipelines and current resource bindings with generation-safe reload |
| `REN-SCENE-01` through `REN-SCENE-07`, `REN-SCENE-10`, scene/view `REN-OWN-*` | [Scene and View Preparation](SceneAndViewPreparation/README.md) | persistent scene data becomes separately owned prepared scene, prepared view, and one coherent GPU-scene publication |
| `REN-VIS-*` | [Visibility and Draw Preparation](GeometryAndResources/VisibilityAndDrawPreparation.md) | one prepared scene/view becomes a deterministic validated visible-instance and compatible raster-batch list |
| `REN-SCENE-08`, `REN-SCENE-09`, residency observations in `REN-DIAG-*` | [Mesh and Texture Residency](GeometryAndResources/MeshAndTextureResidency.md) | immutable asset generations become active GPU resources and retire within explicit concurrency, byte, and lifetime bounds |
| `REN-TEMP-*` | [Temporal Sampling and History](FrameExecution/TemporalSamplingAndHistory.md) | one per-view sample/history identity drives jitter, previous transforms, invalidation, motion, reprojection, and temporal consumers |
| `REN-RESO-*`, related `REN-TEMP-*` and `REN-POST-04` through `REN-POST-06` | [Resolution, Sampling, and Anti-Aliasing](PostProcessing/ReconstructionAndGeneration/ResolutionSamplingAndAntiAliasing.md) | output/render extents, sample policy, provider resolution, resize invalidation, and explicit absent AA/dynamic-resolution modes remain one truthful contract |
| `REN-MAT-*`, `REN-GBUF-*`, `REN-FRONT-*` | [Geometry, Materials, and GBuffer](GeometryAndResources/GeometryMaterialsAndGBuffer.md) | visible surface geometry becomes the shared deferred material/depth/motion contract |
| `REN-PBR-*`, shared `REN-LGT-*` | [Lighting](Lighting/README.md) | selects one surface-lighting mode and joins five direct/indirect lobes, emissive, and sky into scene color |
| direct portions of `REN-PBR-*` and `REN-LGT-*` | [Direct Lighting](Lighting/DirectLighting.md) | analytic lights and visibility become direct diffuse, specular, and subsurface radiance |
| indirect portions of `REN-PBR-*` and `REN-LGT-*` | [Indirect Lighting](Lighting/IndirectLighting.md) | secondary surface transport and history become indirect diffuse/specular radiance |
| `FCR-REN-08`, `PTD-*`, eventual `OPT-*` | [Offline Path Tracer](Lighting/OfflinePathTracer/README.md) | feature definition, discovery gate, and eventual proof contract for an independent offline reference oracle |
| `REN-VOL-*` | [Volumetric Lighting](Lighting/VolumetricLighting.md) | explicit negative capability boundary for media, fog, scattering, atmosphere, and aerial perspective |
| `REN-DECAL-*` | [Deferred Decals](DeferredDecals/README.md) | explicit current absence plus a separately labeled [composition architecture](DeferredDecals/CompositionArchitecture.md) |
| `REN-RT-*` | [Ray Tracing](RayTracing/README.md) | prepared scene identity becomes classic/PTLAS acceleration and inline/native effect traversal; target invariants are separate |
| `REN-POST-*` | [Post Processing](PostProcessing/README.md) | one ordered image-space family transforms scene-linear lighting into a resolved, encoded output while naming absent expected stages |
| `REN-POST-01` through `REN-POST-03` | [Exposure](PostProcessing/DisplayPipeline/Exposure.md) | scene luminance and per-view settings become one bounded history-aware exposure value |
| `REN-POST-04` through `REN-POST-06` | [Image Reconstruction and Upscaling](PostProcessing/ReconstructionAndGeneration/ImageReconstructionAndUpscaling.md) | render-extent color and guides become one output-extent resolved color |
| `REN-POST-07` | [Tone Mapping](PostProcessing/DisplayPipeline/ToneMapping.md) | exposure-weighted scene-referred HDR becomes display-linear color through one of three operators |
| `REN-POST-11` | [Color Grading](PostProcessing/DisplayPipeline/ColorGrading.md) | explicit negative boundary for grading controls, transforms, and LUT workflows |
| `REN-POST-12` | [Chromatic Aberration](PostProcessing/DisplayPipeline/ChromaticAberration.md) | explicit negative boundary for a lens/channel distortion effect |
| `REN-POST-13` | [Frame Generation](PostProcessing/ReconstructionAndGeneration/FrameGeneration.md) | explicit negative boundary for generated-frame synthesis, pacing, UI, and presentation |
| `REN-POST-08` through `REN-POST-10` | [Presentation and Output](PostProcessing/DisplayPipeline/PresentationAndOutput.md) | display-linear/debug color becomes one encoded back-buffer or viewport product |
| `REN-UI-*` | [UI and Viewport Composition](ViewportAndDiagnostics/UiAndViewportComposition.md) | immutable UI packets or viewport textures are composed after graph execution and before submission |
| `REN-LAT-*` | [Latency Coordination](FrameExecution/LatencyCoordination.md) | one logical frame identity joins host simulation with capability-gated D3D12 submit/present PCL markers and optional Reflex sleep |
| `REN-DBG-*` | [Debug Views](DebugViews/README.md) | produced GBuffer, lighting, or scene data becomes a selected diagnostic visualization; view-mode/show-flag control and display-domain correction have separate target owners inside the dossier |
| `REN-DIAG-01` through `REN-DIAG-07` | [Diagnostics, Products, and Capture](ViewportAndDiagnostics/DiagnosticsProductsAndCapture.md) | bounded observations and render products become actionable diagnostics or asynchronous captures |
| `REN-SET-*` | [Settings State and Persistence](RuntimeConfiguration/SettingsStateAndPersistence.md) | aggregate requested settings move through workspace persistence, startup/editor commit, and serial/render-thread CVar application |
| cross-feature shader programs | [Shader Program Catalog](ShaderRuntime/ShaderProgramCatalog.md) | exact registration, stage, entry, consumer, binding-boundary, and required-target accounting for every Renderer global program |
| cross-feature reachability | [Feature Selector Catalog](RuntimeConfiguration/FeatureSelectorCatalog.md) | exact CVar/settings request, persistence, active consumer, fallback, restart, and absence coverage |

## Public Facade Operation Coverage

This table closes the public-method audit independently from the source-folder audit. A facade method may route to multiple feature owners, but none may remain documented only as `REN-OWN-01` vocabulary.

| `Renderer` public operation | Runtime owner/result | Feature contract |
| --- | --- | --- |
| construction/destruction | initializes application-thread external runtime plus serial/threaded coordinator/host; shutdown settles and destroys in reverse ownership order | [Rendering a Sparkle Frame](../RenderingASparkleFrame.md) and [Latency Coordination](FrameExecution/LatencyCoordination.md) |
| `SubmitViewportRenderRequest` | stages swapchain/offscreen viewport target, identity, extent, and display intent | [Diagnostics, Products, and Capture](ViewportAndDiagnostics/DiagnosticsProductsAndCapture.md), [Presentation and Output](PostProcessing/DisplayPipeline/PresentationAndOutput.md), and [UI/Viewport Composition](ViewportAndDiagnostics/UiAndViewportComposition.md) |
| `SubmitRenderFrame` | stages immutable scene/view submission and monotonic frame identity | [Rendering a Sparkle Frame](../RenderingASparkleFrame.md) and [Scene/View Preparation](SceneAndViewPreparation/README.md) |
| `SubmitUiRenderPacket` | stages immutable host/editor UI replay data | [UI and Viewport Composition](ViewportAndDiagnostics/UiAndViewportComposition.md) |
| `SubmitRenderingSettings` | applies directly or queues an aggregate state to the render execution context | [Settings State and Persistence](RuntimeConfiguration/SettingsStateAndPersistence.md) and [Feature Selector Catalog](RuntimeConfiguration/FeatureSelectorCatalog.md) |
| `BeginSimulationFrame`, `EndSimulationFrame` | forwards host simulation boundaries to the optional external latency runtime | [Latency Coordination](FrameExecution/LatencyCoordination.md) |
| `GetViewportRenderProducts` | returns generation-qualified published viewport resources | [Diagnostics, Products, and Capture](ViewportAndDiagnostics/DiagnosticsProductsAndCapture.md) and [Presentation and Output](PostProcessing/DisplayPipeline/PresentationAndOutput.md) |
| `ReloadShaders`, `GetShaderGeneration` | requests complete runtime generation replacement and observes active identity | [Pipeline Materialization and Typed Binding](ShaderRuntime/PipelineMaterializationAndTypedBinding.md) and [Shader Program Catalog](ShaderRuntime/ShaderProgramCatalog.md) |
| `CaptureMeshDiagnostics` | snapshots mesh cache/residency state | [Mesh and Texture Residency](GeometryAndResources/MeshAndTextureResidency.md) and [Diagnostics, Products, and Capture](ViewportAndDiagnostics/DiagnosticsProductsAndCapture.md) |
| `CaptureMeshPreview` | resolves a CPU/editor preview for one runtime mesh identity | [Diagnostics, Products, and Capture](ViewportAndDiagnostics/DiagnosticsProductsAndCapture.md) |
| `CaptureTextureDiagnostics` | snapshots texture cache/residency and preview handles | [Mesh and Texture Residency](GeometryAndResources/MeshAndTextureResidency.md) and [Diagnostics, Products, and Capture](ViewportAndDiagnostics/DiagnosticsProductsAndCapture.md) |
| `CaptureMemoryDiagnostics` | combines Renderer/RHI memory observations | [Mesh and Texture Residency](GeometryAndResources/MeshAndTextureResidency.md) and [Diagnostics, Products, and Capture](ViewportAndDiagnostics/DiagnosticsProductsAndCapture.md) |
| `RequestViewportCapture`, `TryTakeViewportCapture` | begins and later moves out an asynchronous product readback | [Diagnostics, Products, and Capture](ViewportAndDiagnostics/DiagnosticsProductsAndCapture.md) |
| `OnRender` | executes the coordinator's next serial/threaded frame action | [Rendering a Sparkle Frame](../RenderingASparkleFrame.md) |

Adding or removing a public method requires this table, `REN-OWN-01`, its owning dossier, the implementation/consumer route, build membership, and evidence mapping to change together.

## Source-Owner Coverage Audit

This matrix closes horizontal discoverability over the current `Engine/Renderer` source tree. It is a navigation audit, not a second capability inventory: capability states and limits remain in [Capability Inventory](../CapabilityInventory.md), and each linked dossier owns the mechanism and proof contract.

| Current source owner | Feature documentation owner | Coverage decision |
| --- | --- | --- |
| `Public/Renderer.h`, `Public/RendererAPI.h`, `Private/Host`, `Private/Commands`, `Private/Concurrency`, `Private/Frame` | [Rendering a Sparkle Frame](../RenderingASparkleFrame.md), [Scene and View Preparation](SceneAndViewPreparation/README.md), [Frame Graph](FrameExecution/FrameGraphAndScheduling.md), [Latency Coordination](FrameExecution/LatencyCoordination.md), and [Settings State and Persistence](RuntimeConfiguration/SettingsStateAndPersistence.md) | public facade/export boundary, admission, serial/threaded coordination, settings and simulation-marker controls, frame identity, execution, submit, and retirement |
| `Public/Concurrency`, `Public/SceneData`, `Public/Meshes`, `Public/Resources`, `Public/Settings`, `Public/Viewport` | [Rendering a Sparkle Frame](../RenderingASparkleFrame.md), [Scene and View Preparation](SceneAndViewPreparation/README.md), [Mesh and Texture Residency](GeometryAndResources/MeshAndTextureResidency.md), [Settings State and Persistence](RuntimeConfiguration/SettingsStateAndPersistence.md), [Geometry/Materials/GBuffer](GeometryAndResources/GeometryMaterialsAndGBuffer.md), [Feature Selector Catalog](RuntimeConfiguration/FeatureSelectorCatalog.md), [Diagnostics, Products, and Capture](ViewportAndDiagnostics/DiagnosticsProductsAndCapture.md) | submitted-frame/control handles, semantic scene payloads, resource/texture and mesh handles, aggregate settings state, and viewport request/product contracts |
| `Public/FrameGraph`, `Public/ShaderParameters` | [Frame Graph and Scheduling](FrameExecution/FrameGraphAndScheduling.md), [Pipeline Materialization and Typed Binding](ShaderRuntime/PipelineMaterializationAndTypedBinding.md), [Shader Program Catalog](ShaderRuntime/ShaderProgramCatalog.md), and the semantic pass dossier | graph declaration handles, pass parameters, shader ABI use, product identity, and execution ownership |
| `Public/Debug`, `Public/Diagnostics`, `Public/Editor`, `Public/UI` | [Debug Views](DebugViews/README.md), [Diagnostics, Products, and Capture](ViewportAndDiagnostics/DiagnosticsProductsAndCapture.md), and [UI and Viewport Composition](ViewportAndDiagnostics/UiAndViewportComposition.md) | public mode, observation, editor integration, immutable UI packet, and texture-binding seams |
| `Private/Scene`, `Private/View`, `Private/Temporal`, `Private/ShaderData` | [Scene and View Preparation](SceneAndViewPreparation/README.md), [Visibility and Draw Preparation](GeometryAndResources/VisibilityAndDrawPreparation.md), [Temporal Sampling and History](FrameExecution/TemporalSamplingAndHistory.md), [Resolution/Sampling/AA](PostProcessing/ReconstructionAndGeneration/ResolutionSamplingAndAntiAliasing.md), [Geometry/Materials/GBuffer](GeometryAndResources/GeometryMaterialsAndGBuffer.md), [Lighting](Lighting/README.md), [Ray Tracing](RayTracing/README.md) | persistent scene versus view ownership, visibility/classification/sort/batch, continuity, extents/sampling, GPU payloads, temporal uniforms, materials, lights, and RT planning |
| `Private/Meshes`, `Private/Textures`, `Private/Resources` | [Mesh and Texture Residency](GeometryAndResources/MeshAndTextureResidency.md), [Scene and View Preparation](SceneAndViewPreparation/README.md), [Geometry/Materials/GBuffer](GeometryAndResources/GeometryMaterialsAndGBuffer.md), [Diagnostics, Products, and Capture](ViewportAndDiagnostics/DiagnosticsProductsAndCapture.md) | admission, decode/preparation, upload, active generations, defaults, histories, owned buffers, budgets, diagnostics, and completion-safe eviction |
| `Private/FrameGraph`, `Private/Frame/Graph` | [Frame Graph and Scheduling](FrameExecution/FrameGraphAndScheduling.md) | declaration, compilation, versions, dependencies, barriers, queues, transients, parallel recording, execution, history, and graph retirement |
| `Private/Passes/Core`, `Private/Pipeline`, `Private/PipelineRuntime`, `Private/ShaderParameters`, `ShaderRegistrations` | [Pipeline Materialization and Typed Binding](ShaderRuntime/PipelineMaterializationAndTypedBinding.md), [Frame Graph](FrameExecution/FrameGraphAndScheduling.md), [Shader Program Catalog](ShaderRuntime/ShaderProgramCatalog.md) | typed parameter binding, render state, program/pipeline materialization/cache, registration membership, hot-swap generation, and retirement |
| `Private/Passes/GBuffer` | [Geometry, Materials, and GBuffer](GeometryAndResources/GeometryMaterialsAndGBuffer.md), [Debug Views](DebugViews/README.md), [Deferred Decals](DeferredDecals/README.md) | raster/ray primary visibility, depth, sky motion, debug production, and explicit absence of decal composition |
| `Private/Passes/Lighting`, `Private/Lighting` | [Lighting](Lighting/README.md) and its Direct, Indirect, Volumetric, and Offline Path Tracer children | surface-lighting modes, lobe targets, reservoirs, visibility, reference accumulation, composite, sky, and explicit non-surface gaps |
| `Private/RayTracing` | [Ray Tracing](RayTracing/README.md), [Geometry/Materials/GBuffer](GeometryAndResources/GeometryMaterialsAndGBuffer.md), [Direct Lighting](Lighting/DirectLighting.md), [Indirect Lighting](Lighting/IndirectLighting.md) | BLAS/TLAS/PTLAS, SBT, capability reporting, effect frontends/settings, and diagnostics routed by semantic consumer |
| `Private/Passes/PostProcessing`, `Private/Passes/Presentation`, `Private/Upscaling`, `Private/RayReconstruction`, `Private/Providers`, `Private/Streamline`, `Private/Integrations` | [Post Processing](PostProcessing/README.md) and its children, [Resolution/Sampling/AA](PostProcessing/ReconstructionAndGeneration/ResolutionSamplingAndAntiAliasing.md), [Temporal Sampling](FrameExecution/TemporalSamplingAndHistory.md), and [Latency Coordination](FrameExecution/LatencyCoordination.md) | exposure, render/output extent, reconstruction/upscale, sampling, tone/output, provider readiness/fallback, provider temporal constants, external runtime interop/latency, and negative AA/post features |
| `Private/Passes/Debug`, `Private/Debug` | [Debug Views](DebugViews/README.md), [Feature Selector Catalog](RuntimeConfiguration/FeatureSelectorCatalog.md) | visualization production plus registered global/session selection; `Private/Debug` is a selector owner, not evidence of a shipped debug product |
| `Private/Diagnostics`, `Private/Viewport` | [Diagnostics, Products, and Capture](ViewportAndDiagnostics/DiagnosticsProductsAndCapture.md), [Presentation and Output](PostProcessing/DisplayPipeline/PresentationAndOutput.md) | frame/pass/memory observation, render-product publication, async readback, format/identity, bounds, and failure |
| `Private/UI`, `Private/Editor`, public UI/viewport/editor contracts | [UI and Viewport Composition](ViewportAndDiagnostics/UiAndViewportComposition.md), [Diagnostics, Products, and Capture](ViewportAndDiagnostics/DiagnosticsProductsAndCapture.md) | immutable UI packet replay, host/editor composition, viewport texture identity, preview/capture consumers, and registry lifetime |
| `Private/Settings`, public settings, Renderer/RHI CVars consumed by settings | [Settings State and Persistence](RuntimeConfiguration/SettingsStateAndPersistence.md), [Feature Selector Catalog](RuntimeConfiguration/FeatureSelectorCatalog.md), and each selected feature dossier | aggregate transport, startup/editor commit, defaults, parsing, per-view resolution, persistence, restart/topology behavior, requested/active result, and known ineffective/absent controls |
| `Engine/Renderer/CMakeLists.txt` and `ShaderRegistrations` | [Capability Inventory](../CapabilityInventory.md), [Shader Program Catalog](ShaderRuntime/ShaderProgramCatalog.md), and applicable provider/post-processing dossiers | main module, shader-registration/cook objects, optional NVIDIA provider target, public/private dependencies, generated support artifacts, and program membership |
| `Engine/Renderer/Tests` | [Validation and Evidence](../../../../../Engineering/Verification/ValidationAndEvidence.md) plus the dossier whose contract a test exercises | currently empty; a directory is not evidence or an independent feature owner |

The empty `Private/Lighting` directory and source-folder names do not create capabilities. Conversely, a reachable selector/provider/public method still requires a row and dossier even when its implementation is distributed across infrastructure folders.

## Documentation Contract Coverage

This table assesses whether the feature-local **definition** is complete enough to execute later; it does not say the feature passed. `Defined` means criteria/checks exist, not that evidence exists.

| Feature owner | Contract-definition state | Next evidence or lifecycle action |
| --- | --- | --- |
| Whole-frame production | Defined, unproved | Execute the `AC-FRM-*`/`FM-FRM-*` contract in [Rendering a Sparkle Frame](../RenderingASparkleFrame.md), including serial/threaded equivalence, identity, publication, retirement, and shutdown. |
| Feature selector catalog | Defined, unproved | Execute the `AC-SEL-*`/`FM-SEL-*` contract in [Feature Selector Catalog](RuntimeConfiguration/FeatureSelectorCatalog.md); registration or persistence alone is not active-feature evidence. |
| Frame graph and scheduling | Defined, unproved | Execute the dossier's dependency, aliasing, queue-ordering, parallel-recording, rebuild, and retirement checks. |
| Pipeline materialization and typed binding | Defined, unproved | Execute `AC-PIP-*`/`FM-PIP-*` across registration/build membership, ABI, cache identity, binding domains, backend lowering, reload, and retirement. |
| Scene and view preparation | Defined, unproved | Execute the adjacent family [Acceptance](SceneAndViewPreparation/Acceptance.md) contract for identity, continuity, culling, deformation, capacity, cancellation, multi-view, publication, and backend joins. |
| Visibility and draw preparation | Defined, unproved | Execute `AC-VIS-*`/`FM-VIS-*` for bounds/frustum classification, candidate identity, group preservation, deterministic sorting, batch equivalence, task failure, diagnostics, and absent advanced paths. |
| Mesh and texture residency | Defined, unproved | Execute `AC-RES-*`/`FM-RES-*` across admission, budgets, CPU work, upload, activation, replacement, cancellation, eviction, and multi-queue retirement. |
| Temporal sampling and history | Defined, unproved | Execute `AC-TMP-*`/`FM-TMP-*` for exact sampling, history/invalidation order, motion/reprojection agreement, multi-view isolation, and both backends. |
| Resolution, sampling, and anti-aliasing | Defined, unproved | Execute `AC-RESO-*`/`FM-RESO-*` for render/output extent agreement, provider resolution, history reset, active single-sample truth, and negative MSAA/post-AA/dynamic-resolution reachability. |
| Geometry/materials/GBuffer | Defined, unproved | Execute the dossier's raster/ray semantic matrix and controlled unsupported material/binding failures. |
| Direct Lighting | Defined, unproved | Execute the stable direct-light ownership, numerical, visibility, backend, and controlled-failure contract. |
| Indirect Lighting | Defined, unproved | Execute the stable transport, history, convergence, reset, bias, backend, and controlled-failure contract. |
| Volumetric Lighting | Defined negative boundary | Execute `REN-E24` to confirm absence/reachability; any future implementation needs a new target and completion contract. |
| Offline Path Tracer | Defined, blocked | Complete the adjacent [Discovery gate](Lighting/OfflinePathTracer/Discovery.md); no implementation plan or oracle claim is authorized yet. |
| Ray Tracing | Defined, unproved | Execute the dossier's capability, parity, SBT, lifetime, backend, and failure checks linked to RHI evidence. |
| Post Processing family | Defined, unproved | Execute `AC-POST-*`/`FM-POST-*` across the child contracts and prove every stage join has one color-domain, extent, history, selector, fallback, and output owner. |
| Exposure | Defined, unproved | Execute the stable metering, finite-value, adaptation, viewport, reset, and async-scheduling contract. |
| Image reconstruction/upscaling | Defined, unproved | Execute the stable provider-readiness, requested/active, input, reset, fallback, backend, package, quality, and performance contract. |
| Tone mapping | Defined, unproved | Execute the stable numerical-curve, finite-value, alpha, exposure-interaction, and color-domain contract for all three operators. |
| Color grading | Defined negative boundary | Execute `REN-E26`; any implementation needs owned transform/LUT authoring, color-space, parameter, editor, and proof contracts. |
| Chromatic aberration | Defined negative boundary | Execute `REN-E27`; any implementation needs owned lens model, placement, sampling, viewport, identity, artifact, and cost contracts. |
| Frame generation | Defined negative boundary | Execute `REN-E28`; do not confuse Reflex/PCL latency markers, temporal upscaling, or Ray Reconstruction with generated frames. |
| Presentation/output | Defined, unproved | Execute the stable encoding, format, HDR rejection, viewport-product, resize, and numerical contract; keep Tone Mapping and Debug Views separate. |
| UI/viewport composition | Defined, unproved | Execute the stable packet, blend/color/DPI, generation, texture-lifetime, resize, and controlled-failure contract. |
| Latency coordination | Defined, unproved | Execute `AC-LAT-*`/`FM-LAT-*` for six-marker identity/order, PCL/Reflex readiness, no-op cells, failure/shutdown, 32-bit token narrowing, and measured-benefit separation. |
| Debug Views | Defined, unproved | Use [Acceptance](DebugViews/Acceptance.md); current result remains blocked until its checks run. |
| Diagnostics, products, and capture | Defined, unproved | Execute the stable truthfulness, provenance, bounds, observer-cost, backend, and controlled-failure contract. |
| Settings state and persistence | Defined with current gaps, unproved | Execute `AC-SET-*`/`FM-SET-*`; durable/error-reporting and package persistence criteria are not satisfied by the current in-place silent writer. |
| Shader program catalog | Defined, unproved | Execute exact membership, stage/count, required-target, and declared-metadata reconciliation without duplicating ShaderCompiler publication or Renderer pipeline acceptance. |
| Deferred Decals | Defined future feature, excluded | Use [Acceptance](DeferredDecals/Acceptance.md) only after roadmap admission and implementation; current dossier must continue to report absence. |

Closing a documentation `Partial` row is evidence-design work under `INV-009`; it does not assert that the feature passed, authorize implementation, or require broad validation. Whole-frame lifecycle criteria remain owned by the parent [Rendering a Sparkle Frame](../RenderingASparkleFrame.md) route and are intentionally not duplicated here.

## Supporting Exact Ledgers

- [Feature Selector Catalog](RuntimeConfiguration/FeatureSelectorCatalog.md) owns the exact current CVar/settings-to-feature route and known ineffective/non-persisted controls.
- [Settings State and Persistence](RuntimeConfiguration/SettingsStateAndPersistence.md) owns aggregate state transport, persistence, startup/editor application, and live/restart lifecycle; it does not duplicate the selector rows.
- [Renderer Capability Inventory](../CapabilityInventory.md) owns row-level implementation states, limits, evidence marks, and explicit non-claims.
- [Shader Program Catalog](ShaderRuntime/ShaderProgramCatalog.md) owns the exact registered program/entry/stage/consumer list.
- [Graphics Feature Coverage Matrix](../../../../CrossModule/GraphicsCoverageMatrix.md) owns comparisons across backend and execution modes.
- [Capability Evidence Plan](../../../../../Plans/CapabilityEvidence.md#renderer-capability-to-evidence-map) owns the smallest unanswered checks.

When a new Renderer selector, pass family, provider, persistent product, or output is added, update the applicable dossier, inventory row, frame-stage link, horizontal coverage, and evidence mapping together.

## Placement And Granularity

- The Renderer root owns only module-level navigation, the whole-frame narrative, and the exact capability ledger.
- A single current feature responsibility stays as one descriptive file directly under `Features`.
- A durable feature family or a feature with multiple knowledge owners receives a named subfolder and `README.md`. The family entry defines shared ordering and invariants; child files own independently selectable or independently provable capabilities. Target architecture uses a descriptive filename such as `ExecutionArchitecture.md` or `PresentationArchitecture.md`.
- Architecture owns each feature's local acceptance contract. Keep compact criteria in the dossier; when the matrix is large, place `Acceptance.md` beside the dossier and route it from the `README.md`. Delivery sequence remains in `Docs/Plans`; candidate results, workload status, and release approval remain in `Docs/Acceptance`.
- A target-only feature still gets a current gap dossier when readers could otherwise mistake its design for shipped behavior. Deferred Decals follows this rule.
- Split by responsibility, not file size. Lighting separates Direct, Indirect, Volumetric, and Offline Reference work. Post Processing separates Exposure, Reconstruction/Upscaling, Tone Mapping, Color Grading, Chromatic Aberration, Frame Generation, and Presentation/Output because their results, ordering, capability states, inputs, algorithms, and evidence differ.
