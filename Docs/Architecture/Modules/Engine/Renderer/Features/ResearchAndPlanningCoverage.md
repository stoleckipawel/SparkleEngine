# Renderer Feature Research And Planning Coverage

**Status:** documentation package plan and current coverage assessment; implementation and feature evidence are unchanged

**Responsibility:** apply the Reference Path Tracer documentation quality bar across Renderer features by selecting the smallest honest research/discovery/architecture/experience/plan package for each family and sequencing remaining depth work

**Authority boundary:** each feature dossier owns its behavior and acceptance; feature-local Discovery owns implementation-shaping decisions; feature-local Research owns precedent; feature-local Plan owns delivery order; [First Release Renderer Plans](../FirstRelease/README.md) own cross-feature release order; this page owns only documentation-depth classification and upgrade sequence

**Verified:** 2026-09-10; source/build truth and the initial documentation inventory were inspected at `669637cf`, the initial package/navigation result was rechecked at `30597d7d`, and the three display packages were structurally and source-link revalidated after deepening at committed revision `ca55e7d8`. All Markdown under `Renderer/Features`, the feature-delivery template, first-release Renderer plans, capability/readiness indexes, and representative current source/build routes were inspected. Concurrent edits under `Lighting/ReferencePathTracer` were treated as user-owned precedent and not modified by this pass.

**Current readiness:** Not applicable — documentation package depth does not change the Renderer **36/100** portfolio projection or any candidate verdict. See [Current Feature Readiness](../../../../../Acceptance/CurrentReadiness.md#renderer).

The goal is repeatable planning depth, not seven files per feature. A full package is warranted when decisions, semantics, system shape, experience, and delivery have independent lifecycles. A mature implemented leaf with stable semantics may remain one deep dossier plus its release closure phase. An explicitly excluded feature should not gain a production plan until scope admits it.

## Outcome Of This Pass

| Result | Disposition |
| --- | --- |
| Reference quality bar | retained the seven-role [Reference Path Tracer](Lighting/ReferencePathTracer/README.md) and [Feature Delivery Package](../../../../../Engineering/Workflow/Templates/FeatureDeliveryPackage.md) as precedent |
| Mandatory absent display features | expanded [Color Grading](PostProcessing/DisplayPipeline/ColorGrading/README.md), [Chromatic Aberration](PostProcessing/DisplayPipeline/ChromaticAberration/README.md), and [HDR Display Output](PostProcessing/DisplayPipeline/HDRDisplayOutput/README.md) into feature-local packages with blocked discovery and conditional delivery |
| Existing substantial packages | preserved [Deferred Decals](DeferredDecals/README.md) and [Debug Views](DebugViews/README.md); their next implementation iteration should add/freeze discovery rather than rewrite stable architecture |
| Implemented/current families | retained their source-backed dossiers, `AC/FM/CHK`, and owning first-release closure phases; create separate Research/Semantics/UX only when an active change introduces a real independent decision |
| Explicit negative capabilities | retained absence contracts for volumetrics, frame generation, advanced visibility/draw, and absent AA paths; no implementation plan is created before roadmap admission |
| Navigation and governance | this page records the 26-family coverage decision so future work can find its correct package without duplicating release plans or current-state authority |

## `ITER-REN-FEATUREDOC-01` Control Record

| Field | Record |
| --- | --- |
| Identity | `ITER-REN-FEATUREDOC-01`; owner: Renderer feature documentation; status: **PASS for documentation planning only**; scope: planning-depth audit and the three absent display packages; start revision `669637cf`, integrated revision `30597d7d`, deepening baseline `ca55e7d8`, with user-owned Reference Path Tracer edits preserved; decision: every remaining family has an honest package route |
| North Star | `NS-OWNERSHIP`, `NS-EVIDENCE`, `NS-ADOPTION`, `NS-SIMPLIFY`; make each future feature change start from explicit decisions, owners, proof, and a small non-duplicated document set |
| Persona targets | `PGE-07`, `PGE-08`, `PGE-09`, `PGE-13`, `PGE-15`: **advance planning/documentation only**; no build, runtime, GPU, colorimetric, performance, package, adoption, or evidence-level increase is claimed |
| Delivery target | `DSP-5`, `DSP-6`, `DSP-7`; `FCR-REN-24`, `FCR-REN-25`, `FCR-REN-26`; preserve all other `FCR-REN-01` through `26` routes and explicit release exclusions |
| Complexity budget | add one root coverage owner; keep each feature folder at seven Markdown siblings or fewer; no new top-level Plans/Research taxonomy, duplicate acceptance ledger, production code, test code, compatibility index, or generic rendering framework |
| Performance | **no runtime exposure** — Markdown/navigation only; no executable path or package content changes |
| Decision | `CHK-FDOC-01` through `06` passed on 2026-09-10; this means package/navigation consistency only and does not pass discovery, implementation, executable evidence, or release acceptance |

## Quality Bar

The Reference Path Tracer package is valuable because each document answers a different question:

| Role | Required when | Sufficient depth test |
| --- | --- | --- |
| dossier/acceptance | every independently meaningful feature | bounded result and non-promises; current truth; selectors; support matrix; binary criteria; failures; checks; conjunctive done |
| discovery | implementation could still choose product scope, correctness, units, ownership, UX, budgets, or evidence | every implementation-shaping choice is accepted or the first production stage remains blocked |
| research | external source can materially change design or evidence | primary/pinned sources; observed fact; permitted transfer; non-inference; rights/provenance |
| semantics | equations, protocol, format, units, deterministic state, or artifact meaning deserve independent review | two independent implementations can agree without inventing behavior |
| execution architecture | several owners/lifetimes/backends/stages join | one owner per state; vertical and horizontal flow; failure/recovery; capacity; package; clean break |
| experience | a person or automation discovers, configures, waits, interrupts, diagnoses, or consumes a result | first use, state/action truth, errors/recovery, accessibility, support, and automation equivalence are testable |
| plan | delivery has several dependent vertical slices | prerequisites, deletions, non-goals, stop rules, defect-detecting exits, and copy-ready prompts are stage-local |

Length is not the bar. A long source ledger can still lack discovery or semantic authority; a compact leaf can be complete when its behavior and lifecycle are genuinely small.

## Package Classes

| Class | Shape | Use |
| --- | --- | --- |
| `A — full/high assurance` | dossier, discovery, research, semantics, execution, UX, plan | reference/oracle, color pipeline, cross-module presentation, sensitive algorithms or product workflows |
| `B — focused substantial` | dossier plus the independently needed architecture/research/acceptance/plan roles | contained feature with meaningful migration but fewer semantic/UX owners |
| `C — current closure` | deep current dossier with embedded acceptance plus owning first-release phase | implemented feature whose next work is verification/hardening rather than an unresolved redesign |
| `D — negative boundary` | explicit absence/exclusion dossier; research only when evaluating admission | expected capability that must not be mistaken for support |

Promote a feature to a larger class when active work introduces an unresolved equation/protocol/format, a new selector/public result, source-to-runtime asset, ownership/lifetime migration, backend semantic fork, user workflow, or feature-completion claim that the current owners cannot review independently.

## Twenty-Six-Family Coverage

| FCR | Feature owner | Current package and delivery authority | Class / next depth action |
| --- | --- | --- | --- |
| `FCR-REN-01` | whole-frame production | [Rendering A Sparkle Frame](../RenderingASparkleFrame.md), [Frame And Scene `FS-1`](../FirstRelease/FrameAndScene.md#fs-1--frame-admission-and-coordination) | `C`; add research only for a new scheduler/publication model |
| `FCR-REN-02` | scene/view/GPU-scene preparation | [Scene And View Preparation](SceneAndViewPreparation/README.md), local [Acceptance](SceneAndViewPreparation/Acceptance.md), `FS-2` | `C`; split semantics only if identity/deformation/publication rules materially change |
| `FCR-REN-03` | frame graph and scheduling | [Frame Graph And Scheduling](FrameExecution/FrameGraphAndScheduling.md), `FS-3` | `C`; discovery is required before queue/aliasing/scheduler redesign |
| `FCR-REN-04` | raster/GBuffer surfaces | [Geometry, Materials, And GBuffer](GeometryAndResources/GeometryMaterialsAndGBuffer.md), `GR-2` | `C`; use a semantic companion when the material/GBuffer contract changes |
| `FCR-REN-05` | ray-traced GBuffer | [Ray Tracing](RayTracing/README.md), [Execution Architecture](RayTracing/ExecutionArchitecture.md), `GR-4` | `B`; next active dual-execution change should add Research, Discovery, and Plan before code |
| `FCR-REN-06` | direct lighting | [Direct Lighting](Lighting/DirectLighting.md), `LGT-1` | `C`; promote to `A/B` for estimator/unit/sampling redesign |
| `FCR-REN-07` | indirect lighting | [Indirect Lighting](Lighting/IndirectLighting.md), `LGT-2` | `C`; promote for estimator/reservoir/bias changes |
| `FCR-REN-08` | Reference Path Tracer | [full feature package](Lighting/ReferencePathTracer/README.md), `PTD-00`, `LGT-3` | `A`; precedent; production remains blocked by its own discovery |
| `FCR-REN-09` | exposure | [Exposure](PostProcessing/DisplayPipeline/Exposure.md), `DSP-2` | `C`; separate Semantics if metering/adaptation equations change |
| `FCR-REN-10` | reconstruction/providers | [Image Reconstruction And Upscaling](PostProcessing/ReconstructionAndGeneration/ImageReconstructionAndUpscaling.md), `DSP-3` | `C/B`; provider/version/product admission change requires discovery/research refresh |
| `FCR-REN-11` | debug views and capture | [Debug Views package](DebugViews/README.md), `RD-3` | `B`; add one Discovery file when implementation starts; folder budget permits one |
| `FCR-REN-12` | TLAS and ray scene | [Ray Tracing](RayTracing/README.md), [Execution Architecture](RayTracing/ExecutionArchitecture.md), `GR-3` | `B`; share the future ray-tracing discovery/research package, not a second TLAS plan |
| `FCR-REN-13` | UI/viewport composition | [UI And Viewport Composition](ViewportAndDiagnostics/UiAndViewportComposition.md), `RD-4` | `C`; add UX only if the product-facing interaction expands |
| `FCR-REN-14` | tone mapping | [Tone Mapping](PostProcessing/DisplayPipeline/ToneMapping.md), `DSP-4` | `C`; semantic contract required before changing operators/color domain |
| `FCR-REN-15` | presentation/output | [Presentation And Output](PostProcessing/DisplayPipeline/PresentationAndOutput.md), `DSP-4` | `C`; HDR has its separate `A` package below |
| `FCR-REN-16` | pipeline materialization/binding | [Pipeline Materialization And Typed Binding](ShaderRuntime/PipelineMaterializationAndTypedBinding.md), `RD-1` | `C`; discovery required for ABI/cache/generation redesign |
| `FCR-REN-17` | mesh/texture residency | [Mesh And Texture Residency](GeometryAndResources/MeshAndTextureResidency.md), `GR-1` | `C`; add plan/research for streaming/eviction architecture changes |
| `FCR-REN-18` | temporal sampling/history | [Temporal Sampling And History](FrameExecution/TemporalSamplingAndHistory.md), `DSP-1` | `C`; semantics companion required for jitter/motion/history convention changes |
| `FCR-REN-19` | settings/persistence | [Settings State And Persistence](RuntimeConfiguration/SettingsStateAndPersistence.md), `RD-2` | `C`; UX/plan split only for a product settings redesign |
| `FCR-REN-20` | latency coordination | [Latency Coordination](FrameExecution/LatencyCoordination.md), `RD-5` | `C/B`; provider timing-contract changes require current primary-source research |
| `FCR-REN-21` | visibility/draw preparation | [Visibility And Draw Preparation](GeometryAndResources/VisibilityAndDrawPreparation.md), `GR-2` | `C`; each admitted LOD/occlusion/GPU-driven/multiview capability needs its own discovery decision |
| `FCR-REN-22` | resolution/sampling/AA | [Resolution, Sampling, And Anti-Aliasing](PostProcessing/ReconstructionAndGeneration/ResolutionSamplingAndAntiAliasing.md), `DSP-1` | `C`; absent AA/dynamic-resolution modes remain negative until admitted |
| `FCR-REN-23` | deferred decals | [focused package](DeferredDecals/README.md), `GR-5` | `B`; add Discovery before Phase 1; add Semantics only if composition rules cannot remain in architecture |
| `FCR-REN-24` | color grading | [full package](PostProcessing/DisplayPipeline/ColorGrading/README.md), `CGRD-00`, `DSP-5` | `A`; Stage 0 only until decisions pass |
| `FCR-REN-25` | chromatic aberration | [full package](PostProcessing/DisplayPipeline/ChromaticAberration/README.md), `CHRD-00`, `DSP-6` | `A`; standalone UX is justified by active-state/artifact-classification workflow; Stage 0 only |
| `FCR-REN-26` | HDR Display Output | [full package](PostProcessing/DisplayPipeline/HDRDisplayOutput/README.md), `HDRD-00`, `DSP-7` | `A`; Stage 0 only until platform/color/hardware decisions pass |

## Explicit Negative Capabilities

| Capability | Current owner | Planning rule |
| --- | --- | --- |
| volumetric lighting/media | [Volumetric Lighting](Lighting/VolumetricLighting.md) | `D`; keep absence/exclusion proof. Create discovery/research before roadmap admission, not a speculative implementation plan now. |
| frame generation | [Frame Generation](PostProcessing/ReconstructionAndGeneration/FrameGeneration.md) | `D`; latency/provider adjacency is not support. Admission requires product, platform, fallback, pacing, UI, and evidence discovery. |
| non-ray lighting fallback | [Lighting](Lighting/README.md) | `D`; admit as a separate traversal/product decision rather than hiding fallback inside direct/indirect plans. |
| occlusion, LOD, GPU-driven/indirect, stereo, multiview | [Visibility And Draw Preparation](GeometryAndResources/VisibilityAndDrawPreparation.md) | `D` per capability; do not create one generic “advanced visibility” plan. |
| Renderer MSAA, standalone TAA/FXAA/SMAA, dynamic resolution | [Resolution, Sampling, And Anti-Aliasing](PostProcessing/ReconstructionAndGeneration/ResolutionSamplingAndAntiAliasing.md) | `D` per mode; current reconstruction/jitter infrastructure does not admit them. |

## Upgrade Sequence

### Wave 1 — Mandatory Absent Display Features

Complete in this pass at the documentation-contract level: Color Grading, Chromatic Aberration, and HDR Display Output now have seven-role packages with blocked discovery, primary-source research, executable semantic candidates, explicit ownership/lifetime/failure architecture, full user experience, conditional staged plans, and cross-document traceability. This does not close their discovery gates or readiness.

### Wave 2 — Next Active High-Risk Feature

Choose exactly one when implementation priority is known:

1. **Ray Tracing** — add revision-pinned Unreal/vendor/API source research, a discovery gate spanning inline/native pipeline and SBT/effect parity, and one staged plan shared by `FCR-REN-05/12`.
2. **Deferred Decals** — add a discovery gate that freezes receiver/material/blend/ray-hit/authoring/evidence decisions before existing Phase 1.
3. **Debug Views** — add a discovery gate that freezes signal-domain classification, unavailable state, per-view UX, capture sidecars, and observer-cost budgets before its existing plan.

Do not open all three packages speculatively. Their existing architecture remains useful, and active-source drift should be resolved when one becomes the next implementation slice.

### Wave 3 — Current-Feature Closure

Execute the owning First Release `*-0` inventory/freeze phase before code. Promote an implemented feature from `C` only when that pass finds a real independent research, semantic, architecture, or UX decision. Otherwise keep the dossier plus release phase as the one planning route and spend effort on candidate-bound evidence rather than documentation volume.

### Wave 4 — Excluded Capability Admission

Only roadmap/product admission opens a `D` capability. Its first artifact is Discovery and Research, not a production plan. Admission must add/update the FCR, dependencies, risks, selectors, feature-local acceptance, release matrices, and invalidated evidence.

## Risks For This Documentation Program

| ID | Risk | Prevention, detection, contingency, owner, retirement |
| --- | --- | --- |
| `RISK-FDOC-01` | mass-producing generic files hides rather than closes decisions | use package classes and promotion triggers; delete empty/repeated roles; documentation owner retires when every file has independent responsibility |
| `RISK-FDOC-02` | feature plans duplicate First Release sequencing | feature plans own internal stages and link the mother phase; link audit detects copied authority; feature/release plan owners reconcile before handoff |
| `RISK-FDOC-03` | research is mistaken for local support | every Research header and source row carries non-claims; capability/readiness checks remain unchanged; dossier owner retires after status-language review |
| `RISK-FDOC-04` | detailed target documents silently settle unresolved product/math/platform choices | Discovery owns explicit gates and blocks production stages; placeholder/blocked-decision audit detects leakage |
| `RISK-FDOC-05` | concurrent Reference Path Tracer edits are overwritten | exclude that package from edits, recheck status/scoped diff; root agent owns preservation through handoff |
| `RISK-FDOC-06` | moved display paths leave stale navigation | repository-wide old-path, link, anchor, index, and direct-sibling checks; documentation owner retires after `CHK-FDOC-02/03` |

## Documentation Acceptance And Checks

| ID | Pass criterion | Check |
| --- | --- | --- |
| `AC-FDOC-01` | all 26 FCR families and material explicit negatives have one package class and next-action rule | `CHK-FDOC-01` coverage reconciliation against readiness/FCR/feature indexes |
| `AC-FDOC-02` | each new display package has one owner for dossier, discovery, research, semantics, architecture, UX where independent, and plan | `CHK-FDOC-02` headers/routes/sibling-budget and duplicate-authority review |
| `AC-FDOC-03` | primary-source claims are pinned or publisher-dated and state permitted transfer/non-inference | `CHK-FDOC-03` source/link/provenance review |
| `AC-FDOC-04` | unresolved color/platform/product/evidence choices remain visibly blocked and no production stage is accidentally authorized | `CHK-FDOC-04` blocked-decision/plan-stage/placeholder audit |
| `AC-FDOC-05` | every moved page and new document is reachable from the nearest index and all local links/anchors resolve | `CHK-FDOC-05` repository Markdown link and stale-path validation |
| `AC-FDOC-06` | user-owned Reference Path Tracer changes and unrelated work remain untouched | `CHK-FDOC-06` start/end status and scoped diff review |

| Failure ID | Failure | Required result |
| --- | --- | --- |
| `FM-FDOC-01` | a feature is omitted or assigned a full package only for visual consistency | coverage check fails; add the missing owner or reduce the package with rationale |
| `FM-FDOC-02` | research/target/plan wording upgrades source absence into implementation or evidence | status-language check fails; restore exact evidence boundary |
| `FM-FDOC-03` | old display paths, broken links, duplicate phase lists, or more than seven direct Markdown siblings survive | navigation/granularity check fails until reconciled |
| `FM-FDOC-04` | a discovery-blocked choice appears as an unconditional implementation prompt instruction | plan/discovery trace check fails; move the choice to Discovery and block the stage |
| `FM-FDOC-05` | concurrent user-owned changes overlap or are reformatted | scoped-diff check blocks handoff until ownership is reconciled |

`CHK-FDOC-01` compares `FCR-REN-01` through `26`, Current Readiness, feature index, and this table. `CHK-FDOC-02` inspects headers, roles, direct siblings, and duplicated claims. `CHK-FDOC-03` resolves external/local source links and revision identifiers. `CHK-FDOC-04` searches placeholders, vague status, decision IDs, plan prerequisites, and copy-ready prompts. `CHK-FDOC-05` runs local Markdown path/anchor and old-path checks plus strict UTF-8/whitespace validation. `CHK-FDOC-06` compares start/end revision, status, and scoped diff, followed by `git diff --check`.

## Validation Result

| Check | 2026-09-10 result |
| --- | --- |
| `CHK-FDOC-01` | `FCR-REN-01` through `26` were all present exactly as covered families; material negative capabilities retain explicit `D` rules. |
| `CHK-FDOC-02` | Color Grading, Chromatic Aberration, and HDR Display Output each have seven direct Markdown siblings; every file declares status, responsibility, and authority, and no package exceeds the seven-sibling budget. |
| `CHK-FDOC-03` | all 20 cited external URLs returned HTTP `200`; the six repository `HEAD` revisions matched the hashes recorded in Research; rights, permitted transfer, and non-inference boundaries are explicit. |
| `CHK-FDOC-04` | all expected discovery, acceptance, failure, and check ID ranges were present; prohibited authoring placeholders were absent; all three production plans remain gated by `CGRD-00`, `CHRD-00`, or `HDRD-00`. |
| `CHK-FDOC-05` | repository-wide local Markdown validation found zero missing paths and zero missing anchors; the only old `.md` display paths are verbatim historical status evidence in `ProductAndDelivery.md`; strict UTF-8 found zero failures; `git diff --check` found no whitespace errors. |
| `CHK-FDOC-06` | start/end revision and status were rechecked; the concurrent Reference Path Tracer package was excluded from this pass's edits and preserved when revision `30597d7d` integrated both workstreams. |

## Deepening Validation Result

This second pass was revalidated at committed revision `ca55e7d8` with concurrent user-owned Reference Path Tracer implementation/refactor work left untouched.

| Check | 2026-09-10 deepening result |
| --- | --- |
| feature-local ID and no-orphan inventory | expected owner series were present for 34 blocking decisions, 30 discovery checks, 33 risks, 36 retained hand cases, 24 research source IDs, and all feature acceptance/failure/check series |
| `CHK-FDOC-02/04` | all three packages have exactly seven direct Markdown siblings; 21/21 files declare status/responsibility/authority; 90 required depth-section classes were present; placeholder scan found zero `TODO`/`TBD`/`FIXME`/`XXX`/`PLACEHOLDER` hits; every production plan remains discovery-gated |
| `CHK-FDOC-03` | all 28 unique external primary-source URLs in the three packages returned HTTP `200`, including revision-pinned implementation files and the OpenColorIO security advisory; transfer/non-inference and rights boundaries remain explicit |
| `CHK-FDOC-05` scoped | all 229 local links/anchors in the 21 package files plus the parent/coverage indexes resolved; strict UTF-8 decoded all 23 files |
| `CHK-FDOC-05` repository audit | 217 files/3,086 local links were checked; nine stale links remain outside this pass in concurrently changing Reference Path Tracer/lighting/selector documentation because their linked implementation files or one historical discovery anchor are being replaced; this pass did not edit or claim those routes fixed |
| `CHK-FDOC-06` | direct `ArchitectureBoundaryCheck.cmake` execution passed with no new violations; scoped and full `git diff --check` exited zero; current revision/status and scoped diff were rechecked, and all unrelated concurrent source/documentation changes were preserved |

No engine build, shader compile, runtime, native HDR activation, GPU, display measurement, visual, performance, package, or release check was run or implied. The three feature discovery gates and FCRs remain `Blocked`.

## Completion Rule

This documentation program is healthy when every active feature begins from the package class appropriate to its real risk, not when every feature has the same file count. `ITER-REN-FEATUREDOC-01` may pass after the checks above, but that verdict means only that the planning routes are complete and navigable. It does not pass `CGRD-00`, `CHRD-00`, `HDRD-00`, any `FCR-*`, or any executable release gate.
