# Renderer Feature Dossiers

Status: Renderer feature index; routes current source claims and design intent, not release approval

Scope: give each independently meaningful Renderer feature family an explicit definition and owning document, while the parent inventory retains the exact row ledger

Authority boundary: these dossiers own how Renderer features work, why their boundaries exist, how their stages interact, and what feature-local criteria, controlled failures, checks, and completion definition apply. [Capability Inventory](../CapabilityInventory.md) owns the compact implementation-state/evidence ledger; [Feature Selector Catalog](FeatureSelectors.md) owns exact reachability; [Acceptance](../../../../../Acceptance/README.md) owns cross-feature reports, workload/release gates, and actual candidate verdicts. Context repeated in a dossier explains its feature and does not independently promote the corresponding row.

## Dossier Contract

A Renderer feature dossier must answer—or explicitly mark unanswered—what result the feature produces; how it is selected; requested versus active behavior; inputs and outputs; scene/view/frame ownership; pass and shader stages; RHI/backend prerequisites; history and retirement; limits and unsupported combinations; failure/recovery/diagnostics; design intent and tradeoffs; exact source route; capability IDs; acceptance criteria; controlled failure modes; checks; definition of done; and release-report destination. Unknowns remain named gaps rather than implied support.

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
| `REN-DIAG-*` | [Diagnostics and Capture](DiagnosticsAndCapture.md) | bounded observations and render products become actionable diagnostics or asynchronous captures |

## Documentation Contract Coverage

This table assesses whether the feature-local **definition** is complete enough to execute later; it does not say the feature passed. `Defined` means criteria/checks exist, not that evidence exists.

| Feature owner | Contract-definition state | Action still required |
| --- | --- | --- |
| Whole-frame production | Partial | Convert the current failure/evidence routes into stable binary frame-lifecycle criteria and controlled shutdown/retirement checks. |
| Frame graph and scheduling | Partial | Add feature-local criteria for dependency compilation, aliasing, queue ordering, parallel recording, rebuild, and retirement. |
| Scene and view preparation | Partial | Add binary generation, continuity, culling, deformation, capacity, cancellation, and multi-view checks. |
| Geometry/materials/GBuffer | Partial | Add one explicit raster/ray semantic matrix and stable controlled failures for unsupported material/binding cases. |
| Direct Lighting | Partial | Current algorithm/limits/failures/evidence are defined; assign stable `AC-*`/`FM-*`/`CHK-*` criteria before candidate execution. |
| Indirect Lighting | Partial | Current transport/history/oracle boundary is defined; assign stable convergence, reset, bias, and backend criteria. |
| Volumetric Lighting | Defined negative boundary | Execute `REN-E24` to confirm absence/reachability; any future implementation needs a new target and completion contract. |
| Offline Path Tracer | Defined, blocked | Complete the adjacent [Discovery gate](Lighting/OfflinePathTracer/Discovery.md); no implementation plan or oracle claim is authorized yet. |
| Ray Tracing | Partial | Convert existing parity/SBT/failure obligations into stable feature-local criteria linked to RHI evidence. |
| Post Processing family | Partial | Preserve one ordered stage contract and require every added stage to declare color domain, extent, history, selector, fallback, and output ownership. |
| Exposure | Partial | Add stable metering, finite-value, adaptation, viewport, reset, and async-scheduling criteria. |
| Image reconstruction/upscaling | Partial | Add stable provider readiness, requested/active, input, reset, fallback, backend, package, quality, and performance criteria. |
| Tone mapping | Partial | Add stable numerical curves, finite-value, alpha, exposure interaction, and color-domain criteria for all three operators. |
| Color grading | Defined negative boundary | Execute `REN-E26`; any implementation needs owned transform/LUT authoring, color-space, parameter, editor, and proof contracts. |
| Chromatic aberration | Defined negative boundary | Execute `REN-E27`; any implementation needs owned lens model, placement, sampling, viewport, identity, artifact, and cost contracts. |
| Frame generation | Defined negative boundary | Execute `REN-E28`; do not confuse Reflex/PCL latency markers, temporal upscaling, or Ray Reconstruction with generated frames. |
| Presentation/output | Partial | Add stable encoding, format, HDR rejection, viewport-product, resize, and numerical criteria; keep Tone Mapping and Debug Views separate. |
| UI/viewport composition | Partial | Add stable packet, blend/color/DPI, generation, texture-lifetime, resize, and failure criteria. |
| Debug Views | Defined, unproved | Use [Acceptance](DebugViews/Acceptance.md); current result remains blocked until its checks run. |
| Diagnostics and capture | Partial | Add stable truthfulness, provenance, bounds, observer-cost, and controlled-failure criteria. |
| Shader program catalog | Partial | Add a feature-local catalog completeness/ABI/publication contract without duplicating ShaderCompiler acceptance. |
| Deferred Decals | Defined future feature, excluded | Use [Acceptance](DeferredDecals/Acceptance.md) only after roadmap admission and implementation; current dossier must continue to report absence. |

Closing a `Partial` row is documentation/evidence-design work under `INV-009`; it does not authorize implementation or broad validation.

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
