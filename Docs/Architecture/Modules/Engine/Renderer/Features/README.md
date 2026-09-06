# Renderer Feature Dossiers

Status: Renderer feature index; routes current source claims and design intent, not release approval

Scope: give each independently meaningful Renderer feature family an explicit definition and owning document, while the parent inventory retains the exact row ledger

Authority boundary: these dossiers own how Renderer features work, why their boundaries exist, how their stages interact, and what feature-local criteria, controlled failures, checks, and completion definition apply. [Capability Inventory](../CapabilityInventory.md) owns the compact implementation-state/evidence ledger; [Feature Selector Catalog](FeatureSelectors.md) owns exact reachability; [Acceptance](../../../../../Acceptance/README.md) owns cross-feature reports, workload/release gates, and actual candidate verdicts. Context repeated in a dossier explains its feature and does not independently promote the corresponding row.

## Dossier Contract

A Renderer feature dossier must answer—or explicitly mark unanswered—what result the feature produces; how it is selected; requested versus active behavior; inputs and outputs; scene/view/frame ownership; pass and shader stages; RHI/backend prerequisites; history and retirement; limits and unsupported combinations; failure/recovery/diagnostics; design intent and tradeoffs; exact source route; capability IDs; acceptance criteria; controlled failure modes; checks; definition of done; and release-report destination. Unknowns remain named gaps rather than implied support.

## Why This Depth And Granularity

- A current-state narrative lets an engineer find the real owner and extension seam without reverse-engineering every pass, while source/build membership remains the implementation authority.
- Vertical traces expose handoff defects in identity, lifetime, publication, failure, and retirement that a flat feature list cannot reveal.
- Horizontal matrices prevent one backend, traversal mode, material/geometry case, viewport, lifecycle state, or provider fallback from lending an unjustified verdict to unlike cells.
- Stable `AC-*`, `FM-*`, and `CHK-*` identities make later evidence falsifiable and reusable without storing candidate results in Architecture.
- Explicit negative dossiers prevent familiar vocabulary, nearby SDK support, dormant code, target designs, or artifacts from being advertised as implemented features.
- Feature-sized ownership keeps the documents maintainable: shared orchestration is described once at the family/frame boundary, while independently selectable or independently failing results own their own contract.

## Feature Map

| Capability families | Feature dossier | Primary result |
| --- | --- | --- |
| `REN-OWN-*` | [Rendering a Sparkle Frame](../RenderingASparkleFrame.md) | accepted submission becomes one submitted/presented frame with explicit ownership and identity |
| `REN-FG-*` | [Frame Graph and Scheduling](FrameGraphAndScheduling.md) | typed feature declarations become synchronized GPU work and completion-owned lifetime |
| `REN-SCENE-*` | [Scene and View Preparation](SceneAndViewPreparation.md) | persistent scene data becomes frame-local prepared scene, view work, and GPU-scene bindings |
| `REN-MAT-*`, `REN-GBUF-*`, `REN-FRONT-*` | [Geometry, Materials, and GBuffer](GeometryMaterialsAndGBuffer.md) | visible surface geometry becomes the shared deferred material/depth/motion contract |
| `REN-PBR-*`, shared `REN-LGT-*` | [Lighting](Lighting/README.md) | selects one surface-lighting mode and joins five direct/indirect lobes, emissive, and sky into scene color |
| direct portions of `REN-PBR-*` and `REN-LGT-*` | [Direct Lighting](Lighting/DirectLighting.md) | analytic lights and visibility become direct diffuse, specular, and subsurface radiance |
| indirect portions of `REN-PBR-*` and `REN-LGT-*` | [Indirect Lighting](Lighting/IndirectLighting.md) | secondary surface transport and history become indirect diffuse/specular radiance |
| `FCR-REN-08`, `PTD-*`, eventual `OPT-*` | [Offline Path Tracer](Lighting/OfflinePathTracer/README.md) | feature definition, discovery gate, and eventual proof contract for an independent offline reference oracle |
| `REN-VOL-*` | [Volumetric Lighting](Lighting/VolumetricLighting.md) | explicit negative capability boundary for media, fog, scattering, atmosphere, and aerial perspective |
| `REN-DECAL-*` | [Deferred Decals](DeferredDecals/README.md) | explicit current absence plus a separately labeled [composition architecture](DeferredDecals/CompositionArchitecture.md) |
| `REN-RT-*` | [Ray Tracing](RayTracing/README.md) | prepared scene identity becomes classic/PTLAS acceleration and inline/native effect traversal; target invariants are separate |
| `REN-POST-*` | [Post Processing](PostProcessing/README.md) | one ordered image-space family transforms scene-linear lighting into a resolved, encoded output while naming absent expected stages |
| `REN-POST-01` through `REN-POST-03` | [Exposure](PostProcessing/Exposure.md) | scene luminance and per-view settings become one bounded history-aware exposure value |
| `REN-POST-04` through `REN-POST-06` | [Image Reconstruction and Upscaling](PostProcessing/ImageReconstructionAndUpscaling.md) | render-extent color and guides become one output-extent resolved color |
| `REN-POST-07` | [Tone Mapping](PostProcessing/ToneMapping.md) | exposure-weighted scene-referred HDR becomes display-linear color through one of three operators |
| `REN-POST-11` | [Color Grading](PostProcessing/ColorGrading.md) | explicit negative boundary for grading controls, transforms, and LUT workflows |
| `REN-POST-12` | [Chromatic Aberration](PostProcessing/ChromaticAberration.md) | explicit negative boundary for a lens/channel distortion effect |
| `REN-POST-13` | [Frame Generation](PostProcessing/FrameGeneration.md) | explicit negative boundary for generated-frame synthesis, pacing, UI, and presentation |
| `REN-POST-08` through `REN-POST-10` | [Presentation and Output](PostProcessing/PresentationAndOutput.md) | display-linear/debug color becomes one encoded back-buffer or viewport product |
| `REN-UI-*` | [UI and Viewport Composition](UiAndViewportComposition.md) | immutable UI packets or viewport textures are composed after graph execution and before submission |
| `REN-DBG-*` | [Debug Views](DebugViews/README.md) | produced GBuffer, lighting, or scene data becomes a selected diagnostic visualization; target display-domain correction is separate |
| `REN-DIAG-*` | [Diagnostics, Products, and Capture](DiagnosticsProductsAndCapture.md) | bounded observations and render products become actionable diagnostics or asynchronous captures |
| cross-feature shader programs | [Shader Program Catalog](ShaderPrograms.md) | exact registration, stage, variant, ABI, publication, and retirement coverage for every Renderer global program |
| cross-feature reachability | [Feature Selector Catalog](FeatureSelectors.md) | exact CVar/settings request, persistence, active consumer, fallback, restart, and absence coverage |

## Source-Owner Coverage Audit

This matrix closes horizontal discoverability over the current `Engine/Renderer` source tree. It is a navigation audit, not a second capability inventory: capability states and limits remain in [Capability Inventory](../CapabilityInventory.md), and each linked dossier owns the mechanism and proof contract.

| Current source owner | Feature documentation owner | Coverage decision |
| --- | --- | --- |
| `Public/Renderer.h`, `Public/RendererAPI.h`, `Private/Host`, `Private/Commands`, `Private/Concurrency`, `Private/Frame` | [Rendering a Sparkle Frame](../RenderingASparkleFrame.md), [Scene and View Preparation](SceneAndViewPreparation.md), [Frame Graph](FrameGraphAndScheduling.md) | public facade/export boundary, admission, serial/threaded coordination, frame identity, execution, submit, and retirement |
| `Public/Concurrency`, `Public/SceneData`, `Public/Meshes`, `Public/Resources`, `Public/Settings`, `Public/Viewport` | [Rendering a Sparkle Frame](../RenderingASparkleFrame.md), [Scene and View Preparation](SceneAndViewPreparation.md), [Geometry/Materials/GBuffer](GeometryMaterialsAndGBuffer.md), [Feature Selectors](FeatureSelectors.md), [Diagnostics, Products, and Capture](DiagnosticsProductsAndCapture.md) | submitted-frame/control handles, semantic scene payloads, resource/texture and mesh handles, settings, and viewport request/product contracts |
| `Public/FrameGraph`, `Public/ShaderParameters` | [Frame Graph and Scheduling](FrameGraphAndScheduling.md), [Shader Program Catalog](ShaderPrograms.md), and the semantic pass dossier | graph declaration handles, pass parameters, shader ABI use, product identity, and execution ownership |
| `Public/Debug`, `Public/Diagnostics`, `Public/Editor`, `Public/UI` | [Debug Views](DebugViews/README.md), [Diagnostics, Products, and Capture](DiagnosticsProductsAndCapture.md), and [UI and Viewport Composition](UiAndViewportComposition.md) | public mode, observation, editor integration, immutable UI packet, and texture-binding seams |
| `Private/Scene`, `Private/View`, `Private/Temporal`, `Private/ShaderData` | [Scene and View Preparation](SceneAndViewPreparation.md), [Geometry/Materials/GBuffer](GeometryMaterialsAndGBuffer.md), [Lighting](Lighting/README.md), [Ray Tracing](RayTracing/README.md) | persistent scene versus view ownership, continuity, GPU payloads, temporal uniforms, materials, lights, and RT planning |
| `Private/Meshes`, `Private/Textures`, `Private/Resources` | [Scene and View Preparation](SceneAndViewPreparation.md), [Geometry/Materials/GBuffer](GeometryMaterialsAndGBuffer.md), [Diagnostics, Products, and Capture](DiagnosticsProductsAndCapture.md) | mesh/texture load, cache, residency, defaults, histories, owned buffers, budgets, and diagnostics; these are mechanisms beneath scene features, not a second streaming product |
| `Private/FrameGraph`, `Private/Frame/Graph` | [Frame Graph and Scheduling](FrameGraphAndScheduling.md) | declaration, compilation, versions, dependencies, barriers, queues, transients, parallel recording, execution, history, and graph retirement |
| `Private/Passes/Core`, `Private/Pipeline`, `Private/PipelineRuntime`, `Private/ShaderParameters`, `ShaderRegistrations` | [Frame Graph](FrameGraphAndScheduling.md), [Shader Program Catalog](ShaderPrograms.md) | typed parameter binding, render state, program/pipeline materialization/cache, registration membership, hot-swap generation, and retirement |
| `Private/Passes/GBuffer` | [Geometry, Materials, and GBuffer](GeometryMaterialsAndGBuffer.md), [Debug Views](DebugViews/README.md), [Deferred Decals](DeferredDecals/README.md) | raster/ray primary visibility, depth, sky motion, debug production, and explicit absence of decal composition |
| `Private/Passes/Lighting`, `Private/Lighting` | [Lighting](Lighting/README.md) and its Direct, Indirect, Volumetric, and Offline Path Tracer children | surface-lighting modes, lobe targets, reservoirs, visibility, reference accumulation, composite, sky, and explicit non-surface gaps |
| `Private/RayTracing` | [Ray Tracing](RayTracing/README.md), [Geometry/Materials/GBuffer](GeometryMaterialsAndGBuffer.md), [Direct Lighting](Lighting/DirectLighting.md), [Indirect Lighting](Lighting/IndirectLighting.md) | BLAS/TLAS/PTLAS, SBT, capability reporting, effect frontends/settings, and diagnostics routed by semantic consumer |
| `Private/Passes/PostProcessing`, `Private/Passes/Presentation`, `Private/Upscaling`, `Private/RayReconstruction`, `Private/Providers`, `Private/Streamline`, `Private/Integrations` | [Post Processing](PostProcessing/README.md) and its children | exposure, reconstruction, upscale, tone/output, provider readiness/fallback, external runtime interop, and negative expected post features |
| `Private/Passes/Debug`, `Private/Debug` | [Debug Views](DebugViews/README.md), [Feature Selectors](FeatureSelectors.md) | visualization production plus registered global/session selection; `Private/Debug` is a selector owner, not evidence of a shipped debug product |
| `Private/Diagnostics`, `Private/Viewport` | [Diagnostics, Products, and Capture](DiagnosticsProductsAndCapture.md), [Presentation and Output](PostProcessing/PresentationAndOutput.md) | frame/pass/memory observation, render-product publication, async readback, format/identity, bounds, and failure |
| `Private/UI`, `Private/Editor`, public UI/viewport/editor contracts | [UI and Viewport Composition](UiAndViewportComposition.md), [Diagnostics, Products, and Capture](DiagnosticsProductsAndCapture.md) | immutable UI packet replay, host/editor composition, viewport texture identity, preview/capture consumers, and registry lifetime |
| `Private/Settings`, public settings, Renderer/RHI CVars consumed by settings | [Feature Selector Catalog](FeatureSelectors.md) and each selected feature dossier | defaults, parsing, per-view resolution, persistence, restart/topology behavior, requested/active result, and known ineffective/absent controls |
| `Engine/Renderer/CMakeLists.txt` and `ShaderRegistrations` | [Capability Inventory](../CapabilityInventory.md), [Shader Program Catalog](ShaderPrograms.md), and applicable provider/post-processing dossiers | main module, shader-registration/cook objects, optional NVIDIA provider target, public/private dependencies, generated support artifacts, and program membership |
| `Engine/Renderer/Tests` | [Validation and Evidence](../../../../../Engineering/Verification/ValidationAndEvidence.md) plus the dossier whose contract a test exercises | currently empty; a directory is not evidence or an independent feature owner |

The empty `Private/Lighting` directory and source-folder names do not create capabilities. Conversely, a reachable selector/provider/public method still requires a row and dossier even when its implementation is distributed across infrastructure folders.

## Documentation Contract Coverage

This table assesses whether the feature-local **definition** is complete enough to execute later; it does not say the feature passed. `Defined` means criteria/checks exist, not that evidence exists.

| Feature owner | Contract-definition state | Next evidence or lifecycle action |
| --- | --- | --- |
| Whole-frame production | Defined, unproved | Execute the `AC-FRM-*`/`FM-FRM-*` contract in [Rendering a Sparkle Frame](../RenderingASparkleFrame.md), including serial/threaded equivalence, identity, publication, retirement, and shutdown. |
| Feature selectors | Defined, unproved | Execute the `AC-SEL-*`/`FM-SEL-*` contract in [Feature Selectors](FeatureSelectors.md); registration or persistence alone is not active-feature evidence. |
| Frame graph and scheduling | Defined, unproved | Execute the dossier's dependency, aliasing, queue-ordering, parallel-recording, rebuild, and retirement checks. |
| Scene and view preparation | Defined, unproved | Execute the dossier's generation, continuity, culling, deformation, capacity, cancellation, and multi-view checks. |
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
| Debug Views | Defined, unproved | Use [Acceptance](DebugViews/Acceptance.md); current result remains blocked until its checks run. |
| Diagnostics, products, and capture | Defined, unproved | Execute the stable truthfulness, provenance, bounds, observer-cost, backend, reload, and controlled-failure contract. |
| Shader program catalog | Defined, unproved | Execute the feature-local catalog completeness, ABI, publication, reload, and clean-break contract without duplicating ShaderCompiler acceptance. |
| Deferred Decals | Defined future feature, excluded | Use [Acceptance](DeferredDecals/Acceptance.md) only after roadmap admission and implementation; current dossier must continue to report absence. |

Closing a documentation `Partial` row is evidence-design work under `INV-009`; it does not assert that the feature passed, authorize implementation, or require broad validation. Whole-frame lifecycle criteria remain owned by the parent [Rendering a Sparkle Frame](../RenderingASparkleFrame.md) route and are intentionally not duplicated here.

## Supporting Exact Ledgers

- [Feature Selector Catalog](FeatureSelectors.md) owns the exact current CVar/settings-to-feature route and known ineffective/non-persisted controls.
- [Renderer Capability Inventory](../CapabilityInventory.md) owns row-level implementation states, limits, evidence marks, and explicit non-claims.
- [Shader Program Catalog](ShaderPrograms.md) owns the exact registered program/entry/stage/consumer list.
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
