# Renderer

Status: module index and current-system reading route; not executable or release evidence

Scope: explain how Sparkle renders a frame, identify the Renderer features that contribute to it, and route exact capability, design, source, and evidence questions to one owner

Current-state basis: source and build configuration rechecked 2026-09-06 through committed `master` revision `c28b33bd`; executable Renderer source is unchanged from the earlier `8414b5dc` audit; no build, runtime, GPU-validation, visual, performance, or package result is claimed

## Start Here

Read [Rendering a Sparkle Frame](RenderingASparkleFrame.md) first. It follows one accepted frame from immutable world submission through scene/view preparation, frame-graph construction and execution, GBuffer, lighting, post processing, presentation, submission, and retirement. Each stage links to its feature dossier and live source owner.

Use the [Renderer Capability Inventory](CapabilityInventory.md) when you need an exact `REN-*` row, implementation state, limit, or evidence destination. The inventory is the ledger; it is deliberately not the explanation of the renderer.

## Renderer Feature Dossiers

| Feature or system | What Sparkle currently does | Current boundary | Owning document |
| --- | --- | --- | --- |
| Frame production | Accepts monotonic immutable submissions, supports serial or render-thread coordination, prepares one frame slot, executes, submits, and advances completion-owned lifetime. | Source path present; serial/threaded equivalence and shutdown stress remain unproved. | [Rendering a Sparkle Frame](RenderingASparkleFrame.md) |
| Frame graph and GPU scheduling | Declares typed raster/compute/ray/transfer/provider passes, compiles resource dependencies, barriers, transients, queues, recording chunks, and submission batches. | Source path present; native validation, overlap, aliasing, and rebuild-cost evidence remain open. | [Frame Graph and Scheduling](Features/FrameExecution/FrameGraphAndScheduling.md) |
| Pipeline materialization and typed binding | Validates cooked shader metadata and typed pass layouts, materializes graphics/compute/ray pipelines, binds current resources, and swaps whole shader generations. | Source path present; ABI, cache identity, backend parity, build membership, and reload stress remain unproved. | [Pipeline Materialization and Typed Binding](Features/ShaderRuntime/PipelineMaterializationAndTypedBinding.md) |
| Scene and view preparation | Maintains a persistent render-side scene, derives frame-local scene and view state through separate owners, and publishes one coherent GPU-scene generation. | Source path present; capacity, failure, identity, and concurrency behavior need executable evidence. | [Scene and View Preparation](Features/SceneAndViewPreparation/README.md) |
| Visibility and draw preparation | Classifies per-view bounds/materials, validates candidate identities, preserves compatible authored groups, and deterministically sorts/forms raster batches. | CPU frustum path present; equivalence/benefit unproved; occlusion, LOD, GPU-driven/indirect, stereo, and multiview routes absent. | [Visibility and Draw Preparation](Features/GeometryAndResources/VisibilityAndDrawPreparation.md) |
| Mesh and texture residency | Admits bounded asynchronous mesh/texture work through preparation/decode, upload, activation, generation replacement, and completion-safe eviction. | Source path present; pressure behavior, fallback asymmetry, throughput, and retirement need executable evidence. | [Mesh and Texture Residency](Features/GeometryAndResources/MeshAndTextureResidency.md) |
| Temporal sampling and history | Owns per-view Halton jitter, previous-camera state, common history validity/invalidation, and the motion/reprojection convention shared by temporal consumers. | Source path present; exact values, cross-consumer agreement, multi-view isolation, and visual stability remain unproved. | [Temporal Sampling and History](Features/FrameExecution/TemporalSamplingAndHistory.md) |
| Geometry, materials, and GBuffer | Renders static, instanced, skinned, and morphed triangle geometry through raster or ray GBuffer frontends into one deferred material contract. | Opaque and alpha-tested paths exist; transparent blending and broader material lobes are not supported. | [Geometry, Materials, and GBuffer](Features/GeometryAndResources/GeometryMaterialsAndGBuffer.md) |
| Ray-tracing scene and traversal | Builds BLAS/TLAS state and runs ray GBuffer/shadow effects through inline queries or native pipelines over shared scene/material semantics. | Capability-gated; reference and ReSTIR-indirect traversal are inline-only, and parity is unproved. | [Ray Tracing](Features/RayTracing/README.md) and its [execution architecture](Features/RayTracing/ExecutionArchitecture.md) |
| Lighting | Selects one ReSTIR or reference surface-lighting mode and joins direct/indirect lobes, emissive, and sky through one composite contract. | Direct and Indirect paths exist and remain unproved; Volumetric Lighting is absent. | [Lighting](Features/Lighting/README.md) |
| Direct lighting | Samples directional, point, spot, and rect lights; resolves ray-traced visibility; evaluates direct diffuse, specular, and wrap-subsurface lobes. | Both current lighting modes require ray tracing; no shadow-map or non-ray fallback exists. | [Direct Lighting](Features/Lighting/DirectLighting.md) |
| Indirect lighting | Resolves secondary diffuse/specular transport through ReSTIR reuse or the accumulating reference path, then joins sky/environment background. | Secondary traversal is inline-only; convergence, bias, history, and reference-oracle credibility remain open. | [Indirect Lighting](Features/Lighting/IndirectLighting.md) |
| Volumetric lighting | Would own participating media, fog, transmittance, scattering, atmosphere, and aerial perspective. | Not implemented; nearby sky, alpha-mask, and wrap-subsurface paths are explicitly not volumetric support. | [Volumetric Lighting](Features/Lighting/VolumetricLighting.md) |
| Deferred decals | Has an accepted target design for shared raster/ray material composition. | Not implemented; no authored/runtime data, primary GBuffer pass, or secondary-ray application exists. | [Deferred Decals](Features/DeferredDecals/README.md) |
| Post Processing | Orders exposure, reconstruction/upscaling, debug handoff, tone mapping, encoding, and publication; names expected absent stages explicitly. | Implemented and absent capabilities have separate child dossiers; family-wide correctness remains unproved. | [Post Processing](Features/PostProcessing/README.md) |
| Exposure | Resolves manual or automatic metering into one history-aware per-view multiplier before debug replacement and presentation. | Source path present; numerical response, reset, finite bounds, and async overlap remain unproved. | [Exposure](Features/PostProcessing/DisplayPipeline/Exposure.md) |
| Image reconstruction and upscaling | Produces one output-extent resolved color through Linear, NVIDIA DLSS Super Resolution, or ReSTIR-specific DLSS Ray Reconstruction. | Linear baseline exists; NVIDIA routes are capability/backend/package gated and unproved. | [Image Reconstruction and Upscaling](Features/PostProcessing/ReconstructionAndGeneration/ImageReconstructionAndUpscaling.md) |
| Resolution, sampling, and anti-aliasing | Resolves viewport/window output extent, provider-selected render extent, active Halton jitter and current single-sample attachment policy before one resolved output. | Extent path present; RHI sample vocabulary is not Renderer MSAA; standalone TAA/FXAA/SMAA and dynamic resolution are absent. | [Resolution, Sampling, and Anti-Aliasing](Features/PostProcessing/ReconstructionAndGeneration/ResolutionSamplingAndAntiAliasing.md) |
| Tone mapping | Multiplies by exposure and maps HDR scene-referred color to display-linear color through Reinhard, ACES approximation, or ACES fitted filmic. | Source path present; no public bypass and numerical/colorimetric correctness remains unproved. | [Tone Mapping](Features/PostProcessing/DisplayPipeline/ToneMapping.md) |
| Color grading | Would own authored grading transforms, parameters, and LUT workflows independently from tone mapping. | Not implemented; fixed tone-mapper choices are not color grading. | [Color Grading](Features/PostProcessing/DisplayPipeline/ColorGrading.md) |
| Chromatic aberration | Would own intentional channel-dependent lens distortion and its placement/sampling policy. | Not implemented; reconstruction or filtering fringes are not feature support. | [Chromatic Aberration](Features/PostProcessing/DisplayPipeline/ChromaticAberration.md) |
| Frame generation | Would own interpolated-frame synthesis, identity, pacing, UI, latency, and presentation contracts. | Not implemented; Streamline Reflex/PCL, DLSS Super Resolution, and Ray Reconstruction are not frame generation. | [Frame Generation](Features/PostProcessing/ReconstructionAndGeneration/FrameGeneration.md) |
| Presentation and output | Applies output encoding and back-buffer or viewport-product publication after tone mapping. | HDR display output is absent; numerical color and exact debug presentation remain unproved. | [Presentation and Output](Features/PostProcessing/DisplayPipeline/PresentationAndOutput.md) |
| UI and editor viewport composition | Replays immutable UI packets as a host overlay or presents an offscreen viewport product through an ImGui texture binding after scene rendering. | Source path present; blend/color/DPI behavior and long-session editor-texture lifetime are unproved. | [UI and Viewport Composition](Features/ViewportAndDiagnostics/UiAndViewportComposition.md) |
| Latency coordination | Joins host simulation start/end with D3D12 render-submit/present markers and optionally routes them through Streamline PCL with Reflex sleep. | Optional D3D12-only inspected path; host ordering, token narrowing, failures, and any latency benefit remain unproved. | [Latency Coordination](Features/FrameExecution/LatencyCoordination.md) |
| Debug views | Selects 16 final, GBuffer, lighting, and GPU-scene visualizations and sends them through the common presentation path. | Current presentation can alter diagnostic values; the corrected display-domain design is target-only. | [Debug Views](Features/DebugViews/README.md) |
| Diagnostics, viewport products, and capture | Collects frame/pass timing and memory data, publishes editor-facing products, and completes asynchronous texture readbacks. | Source path present; truthfulness, color/format semantics, observer cost, and support UX remain unproved. | [Diagnostics, Products, and Capture](Features/ViewportAndDiagnostics/DiagnosticsProductsAndCapture.md) |
| Settings state and persistence | Captures an aggregate requested state, persists 27 owned CVar names, restores them at startup, and commits edits directly or through the render-thread control route. | Current persistence truncates in place and reports no parse/write failure; requested versus resolved/restart-active state is incomplete. | [Settings State and Persistence](Features/RuntimeConfiguration/SettingsStateAndPersistence.md) and [Selector Catalog](Features/RuntimeConfiguration/FeatureSelectorCatalog.md) |
| Shader program catalog | Enumerates every registered Renderer global program, stage, entry, consumer, binding boundary, and required runtime target variant. | Exact source ledger exists; cook completeness and declared-metadata agreement remain unproved. | [Shader Program Catalog](Features/ShaderRuntime/ShaderProgramCatalog.md) and [Shader Compilation](../../Tools/ShaderCompiler/README.md) |

The [feature dossier index](Features/README.md) maps every Renderer capability family to these documents and states what each dossier must answer.

The [Feature Selector Catalog](Features/RuntimeConfiguration/FeatureSelectorCatalog.md) maps the full current CVar/settings surface to active owners and calls out registered-but-ineffective or non-persisted controls.

## Choose By Question

| Question | Read |
| --- | --- |
| In what order is a frame produced, and why? | [Rendering a Sparkle Frame](RenderingASparkleFrame.md) |
| What exactly is supported, partial, gated, vocabulary-only, or absent? | [Capability Inventory](CapabilityInventory.md) |
| Which source owner implements a stage? | The source-route table in the relevant feature dossier, then verify the linked code/CMake. |
| Which setting/CVar selects a feature, and is it actually consumed or persisted? | [Feature Selector Catalog](Features/RuntimeConfiguration/FeatureSelectorCatalog.md) |
| How are aggregate settings saved, restored, moved to the render thread, and marked pending restart? | [Settings State and Persistence](Features/RuntimeConfiguration/SettingsStateAndPersistence.md) |
| Where are simulation/render/present latency markers and Reflex/PCL boundaries owned? | [Latency Coordination](Features/FrameExecution/LatencyCoordination.md) |
| How do D3D12, Vulkan, raster, inline, and native-pipeline paths differ? | [Graphics Feature Coverage Matrix](../../../CrossModule/GraphicsCoverageMatrix.md) |
| How does a feature cross GameFramework, Renderer, ShaderCompiler, and RHI? | [Graphics Feature Execution Traces](../../../CrossModule/FeatureExecutionTraces.md) |
| What proof is still missing? | [Renderer capability-to-evidence map](../../../../Plans/CapabilityEvidence.md#renderer-capability-to-evidence-map) |
| Can a feature ship? | Its `FCR-REN-*` report in [Feature Completion Reports](../../../../Acceptance/FeatureCompletionReports.md); this Architecture route cannot approve it. |

## Ownership Boundary

Renderer owns scene/view/frame policy, technique selection, graph construction, shader/pipeline use, history, providers, and render products. GameFramework owns world/ECS state and publishes immutable render submissions. RHI owns backend resources, command recording, synchronization, native lowering, presentation, and GPU diagnostics. The binding boundary is [Renderer and RHI](../../../Decisions/RendererRhiBoundary.md).

Current source structure:

```text
RenderCoordinator
  -> RendererHost
  -> FramePipeline
       -> RenderScene + PreparedRenderScene
       -> RenderView + RenderViewState
       -> RenderGpuScene + RenderRayTracingScene
       -> FrameGraph
            -> Passes/<feature>
            -> RHI command and submission services
       -> viewport products / UI / present / retirement
```

## Placement Model

The Renderer root contains only module-level entry documents:

- this reader route;
- [Rendering a Sparkle Frame](RenderingASparkleFrame.md), the canonical current execution narrative;
- [Capability Inventory](CapabilityInventory.md), the exact row/state/evidence ledger.

All feature-owned current behavior, feature-local catalogs, negative capability dossiers, target architectures, and feature-local acceptance contracts live under [Features](Features/README.md). A feature receives a subfolder only when it owns multiple independently maintained documents, such as current behavior plus a target architecture or substantial acceptance matrix. Delivery sequencing remains under [Plans](../../../../Plans/Renderer/README.md); candidate results and release-wide proof orchestration remain under [Acceptance](../../../../Acceptance/Renderer/README.md).
