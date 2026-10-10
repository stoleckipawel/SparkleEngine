# Principal Graphics Feature Delivery Catalog

**Status:** strategic deliverable targets and owner routing; no implementation authorization or acceptance results

**Scope:** turn the [persona](EngineerPersona.md), [requirements](Requirements.md) and [role study](Research/PrincipalGraphicsRoles.md) into measurable, inspectable outputs. [Roadmap](Roadmap.md) owns order; feature dossiers own design and thresholds; Acceptance owns results.

This is a gate-based plan. There are no delivery dates, hour estimates or calendar commitments. `PGD-*` are local strategic output IDs, not new runtime features or replacements for `PGE`, `REL`, `MAP`, `WL`, `CASE` or `FCR` identities.

**Choose the next piece at [Roadmap current/next](Roadmap.md#where-we-are-and-what-is-next).** This catalog defines what the outputs demonstrate; admission/order comes from that one driver. Detailed plans and fillable contribution/transfer cards are routed there. This page does not maintain another active queue or results tracker.

**Owner-directed release refinement:** [Required rendering closure](../Acceptance/FirstRelease.md#required-rendering-closure) makes reference reliability, realtime >=30 FPS, complete SR/RR, paired-backend/frame-graph closure and principal-level whole-frame review mandatory release outcomes. Existing PGD/FCR owners remain authoritative; no new implementation or acceptance is inferred.

## Read This Page

- **Choose an output:** [Deliverables](#deliverables-and-owning-plans); use Roadmap for their order.
- **Prepare a case:** [Three headline cases](#three-headline-cases) and the [measurement card](#measurement-card-required-before-results).
- **Study an example:** [Research-informed exercises](#research-informed-delivery-exercises).

## Deliverables And Owning Plans

### `PGD-01` Trustworthy product and reproduction

Exact approved source/product bytes, cleared rights, package manifest, clean consumer/source-adopter transcripts, negative failures and independent approval. Example: package inventory joining file hash, origin, license and import dependency.

**Evidence boundary:** Build/source paths exist; no conjunctive release acceptance is inferred.

**Delivery route:** [First Release stage briefs](../Architecture/CrossModule/FirstRelease/README.md), `REL-00..11`; [Build/Packaging](../Architecture/Modules/BuildAndPackaging/README.md).

**Requirements:** `PGE-01`, `07`, `13..15`

### `PGD-02` Content-to-correct-pixel and bounded reference

One content lineage case with exact import losses, material/transport domain, analytic/minimal/independent checks, raw reference uncertainty, frozen cameras and failure gallery. Example: a pixel traced from source material through cooked inputs and shader ABI to a finite linear-radiance output.

**Evidence boundary:** Tracer source stages delivered; recovery, estimator/oracle and final adoption evidence remain open.

**Delivery route:** Existing authorized recovery: [Reference Path Tracer plan](../Architecture/Modules/Engine/Renderer/Features/Lighting/ReferencePathTracer/Plan.md); release adoption `PTD-02/03`, `REL-04/05`; subsequent `M1/M2`, `WL-01..03`.

**Requirements:** `PGE-02`, `06..09`, `13`

### `PGD-03` Measured paired-API classical rendering

Matched Windows D3D12/Vulkan quality/cost cases on named hardware; three causal studies including one rejected/inconclusive optimization. Compare per-run distributions, memory and whole-frame interference, with native captures.

**Evidence boundary:** D3D12/Vulkan and rendering paths exist; complete workload evidence is unclaimed.

**Delivery route:** After release and correct references: `M3`, `WL-04`; [Workload Studies](../Architecture/CrossModule/PerformanceDiagnostics/WorkloadStudies/Plan.md).

**Requirements:** `PGE-02`, `05`, `08..10`

### `PGD-04` Difficult graphics incident and useful capture workflow

One reproducible incident: symptom, earliest bad product, competing hypotheses, counterexample, minimal fix/deletion, regression/failure evidence and recovery. Capture artifacts identify the actual request, scene, host interval and tool tuple.

**Evidence boundary:** Provider paths and known failures are documented; source presence is not aggregate provider acceptance.

**Delivery route:** [External Capture plan](../Architecture/CrossModule/PerformanceDiagnostics/ExternalCapture/Plan.md) for its remaining checks; [Workload Studies](../Architecture/CrossModule/PerformanceDiagnostics/WorkloadStudies/Plan.md) for the case.

**Requirements:** `PGE-01`, `05..07`, `09..10`, `13`

### `PGD-05` Useful Python analysis consumer

One CLI consumes immutable workload records, rejects mismatched/partial data, emits inspectable per-run comparisons and plots. Two frozen-input runs reproduce the output identity within the declared numeric policy; no duplicate sampler or exporter.

**Evidence boundary:** Existing narrow scripts do not establish a supported workload analyzer.

**Delivery route:** After `REL-11`, frozen producer input and a real comparison consumer: [Workload Studies plan](../Architecture/CrossModule/PerformanceDiagnostics/WorkloadStudies/Plan.md); [Python dossier](../Architecture/Modules/Tools/PythonAutomationAndAnalysis.md), `PY-03`.

**Requirements:** `PGE-05`, `07..08`, `13`

### `PGD-06` Owned data, neural model and training

One diffuse-indirect denoiser, licensed immutable splits, held-out San Miguel, reproducible environment/recipe, overfit diagnostic, classical/noisy baselines, model card and input/loss ablations. A seed is not a cross-platform reproducibility guarantee.

**Evidence boundary:** Owned dataset/training/model family remains a target, distinct from vendor inference.

**Delivery route:** `REL-11` and `M3`; [Neural discovery](../Architecture/CrossModule/NeuralGraphics/Discovery.md), then [plan](../Architecture/CrossModule/NeuralGraphics/Plan.md) Stages 1–2; `M4`, `WL-05`.

**Requirements:** `PGE-03`, `07..08`, `11..12`

### `PGD-07` Model-to-shader product path

One fixed operator inventory exported and numerically checked against PyTorch and FP32 shaders; typed Renderer scheduling, capability/fallback, resource retirement and package ownership. Then profile precision/layout/fusion independently; preserve the correct baseline.

**Evidence boundary:** Owned lowering/kernels/runtime absent in the existing target dossier; no general ML runtime is requested.

**Delivery route:** Accepted neural discovery and `PGD-06`; [Neural plan](../Architecture/CrossModule/NeuralGraphics/Plan.md) Stages 3–5; [Shader System](../Architecture/CrossModule/ShaderSystem/README.md); `M5`, `WL-06`.

**Requirements:** `PGE-03..04`, `09..10`, `12`

### `PGD-08` Held-out evaluation and transferable explanation

Three headline cases, quality/latency/memory frontier and failure gallery, exact manifests, a report and concise talk/demo. At least one non-author reproduces a headline result without private instructions, and an independent review records feedback and resulting changes.

**Evidence boundary:** No owned neural evaluation/adoption result is claimed.

**Delivery route:** `M6`, `WL-07/08`; [portfolio plan](../Architecture/CrossModule/GraphicsPortfolio/Plan.md) assembles accepted [Neural](../Architecture/CrossModule/NeuralGraphics/Acceptance.md), [Workload Studies](../Architecture/CrossModule/PerformanceDiagnostics/WorkloadStudies/README.md) and [Graphics Workloads](../Acceptance/GraphicsWorkloads.md) evidence.

**Requirements:** `PGE-01`, `03..05`, `08`, `11..13`, `15`

### `PGD-09` Honest ecosystem breadth and influence

Upstream contribution, a second adopter and teaching/mentoring records are separate evidence. Linux implementation, AMD hardware acquisition and AMD profiler work are excluded from this selected program.

**Evidence boundary:** Windows work does not prove another architecture, employment history or sustained external influence. Linux delivery is excluded.

**Delivery route:** Conditional [contribution](../Architecture/CrossModule/GraphicsPortfolio/UpstreamContribution.md) and [knowledge-transfer](../Architecture/CrossModule/GraphicsPortfolio/KnowledgeTransfer.md) cards; [Requirements](Requirements.md#non-repository-requirements).

**Requirements:** `PGE-01`, `07`, `13..15`


Counts here define portfolio coverage, not statistical validity. The three studies and one incident may reuse the same candidate/workload when each answers a distinct question. A deleted negative experiment can be valuable evidence; it cannot stand in for an implemented neural shader path or a passed acceptance criterion.

## Research-Informed Delivery Exercises

These are concrete examples within the nine outputs above, not additional features or mandatory vendor integrations. [Public engineer profiles](Research/GraphicsEngineerProfiles.md) establish attribution; [reference cards](Research/RenderingReferenceExamples.md) record exact code/publication locations. Select the local problem and falsifier before implementation. Numeric tolerances remain in the owning discovery/workload.

| Output / example question | Deliverable a reviewer can inspect | Source precedent and actual owner |
| --- | --- | --- |
| `PGD-02`: Why is this reference pixel trustworthy? | One raw-linear pixel/product lineage, bounded transport/material table, estimator derivation, independent fixture and unconverged/invalid-sample rejection. Preserve the accepted tracer contract and finish its remaining proof. | `NVR-04/05/15/17`; [Reference Path Tracer plan](../Architecture/Modules/Engine/Renderer/Features/Lighting/ReferencePathTracer/Plan.md). |
| `PGD-02/03`: When is reservoir reuse valid? | Proposal/target/normalization and history/shift note; matched direct/indirect baseline; disocclusion/light-edit counterexample and quality/memory/cost comparison. | `NVR-02/03/17`; [Direct discovery](../Architecture/Modules/Engine/Renderer/Features/Lighting/DirectLighting/Discovery.md), [Indirect discovery](../Architecture/Modules/Engine/Renderer/Features/Lighting/IndirectLighting/Discovery.md). |
| `PGD-03`: Are CPU recording and binding work limiting the frame? | One declared study with thread/queue timeline, command-order/resource-retirement explanation, matched scene/settings and before/after per-run evidence. Retain the current production parallel topology. | `NVR-01/07/16`; [Workload Studies](../Architecture/CrossModule/PerformanceDiagnostics/WorkloadStudies/Plan.md). The old CAD sample is deprecated; use its successor for current comparison. |
| `PGD-03/04`: Is shader live state, traffic or code size the actual limiter? | One exact source/bytecode/native join; hypothesis and alternative; counter/ISA evidence where available; quality-equivalent whole-frame result. A failure or inconclusive outcome is useful. | `NVR-18/19`; [Shader System plan](../Architecture/CrossModule/ShaderSystem/Plan.md), [Workload Studies](../Architecture/CrossModule/PerformanceDiagnostics/WorkloadStudies/Plan.md). SER is optional. |
| `PGD-05`: Can another engineer compare the evidence correctly? | One bounded offline CLI, stable original-input hashes, run-level tables/plots, raw/display-domain distinction and mismatch/partial-input controls. | `NVR-12/14`; [Workload Studies](../Architecture/CrossModule/PerformanceDiagnostics/WorkloadStudies/Plan.md), existing Python owner. No embedded viewer or general analyzer platform. |
| `PGD-06/07`: Does a small owned model become correct, useful shader work? | Dataset/model/operator cards, held-out split, FP32 shader conformance, one layout/fusion/precision ablation, resource-retirement/fallback and quality/latency/memory frontier. | `NVR-03/10/11/13`; [Neural discovery](../Architecture/CrossModule/NeuralGraphics/Discovery.md) and [plan](../Architecture/CrossModule/NeuralGraphics/Plan.md). Do not add NeRF or OptiX. |
| `PGD-08/09`: Can knowledge and maintenance transfer? | A short delivered-case exercise, one non-author transcript, independent review with changed decision, and an explicit support/retirement note. Later progression: second distinct consumer plus one feedback-driven post-adoption revision. | `NVR-17/20`; [Neural adoption](../Architecture/CrossModule/NeuralGraphics/Acceptance.md), [Workload Studies](../Architecture/CrossModule/PerformanceDiagnostics/WorkloadStudies/Plan.md), existing support owner. |
| Conditional `PGD-03/09`: Would a GPU-driven geometry or memory change solve a measured problem? | First produce a visibility/draw-granularity or residency-pressure diagnosis. Admit a bounded feature only if the current path cannot meet a frozen consumer need. | `NVR-08/09/16`; [Visibility](../Architecture/Modules/Engine/Renderer/Features/GeometryAndResources/VisibilityAndDrawPreparation.md), [Residency](../Architecture/Modules/Engine/Renderer/Features/GeometryAndResources/MeshAndTextureResidency.md). Meshlets/DGC/new vendor SDKs remain study-only until separately admitted. |

Every selected exercise records the source card, exact assumption, local differences, rights route, falsifier and result owner using the [reference-use record](Research/RenderingReferenceExamples.md#delivery-reference-use-record). An algorithm replacement requires its existing feature gate; a documentation precedent cannot reopen or silently change an accepted scope.

## Three Headline Cases

| Case | What the reviewer should learn | Concrete delivery shape |
| --- | --- | --- |
| Content to correct pixel | How assets, transport, shader data, output domain and reference uncertainty join. | One result card; import/material losses; a raw-output lineage diagram; analytic and scene checks; a failure gallery; code and reproduction links. |
| Classical graphics and causal optimization | Why a frame is limited, what changed, and whether the whole product improved on both APIs. | Baseline and candidate manifests; per-run table and tail plot; before/after native artifacts; queue/resource explanation; measured outcomes and one rejected/inconclusive alternative. |
| Owned neural model to shader | How data/model assumptions become a correct, bounded runtime feature and whether the cost is justified. | Dataset/model card; fixed operator/ABI note; conformance table; held-out comparison; precision/layout ablations; fallback/invalid-artifact demonstration; package/adoption record. |

These are narrative groupings over [canonical `CASE-*` and `WL-*` workloads](../Acceptance/GraphicsWorkloads.md), not replacement acceptance identities. A report states personal ownership explicitly and credits external algorithms, assets, integrations and reviewers.

## Measurement Card Required Before Results

| Field | Required value or example |
| --- | --- |
| Question and hypothesis | “Does reducing descriptor preparation work improve the selected workload without changing the rendered output?” Name a proposed mechanism and an observation that would refute it. |
| Candidate and baseline | Commit/dirty state, cooked input and shader/model hashes, API/profile, driver/GPU/CPU, scene/camera and output settings. |
| Metric definition | Quantity, unit and denominator: per-run p95 GPU frame milliseconds; bytes of peak workspace; reference-domain image error; not unnamed “performance.” |
| Decision rule | Absolute and relative practical-effect bands, uncertainty method, quality tolerance and failure policy frozen by the owning workload/discovery before candidate results. An overlapping interval is inconclusive. |
| Samples and observation | Reuse [Graphics Workloads](../Acceptance/GraphicsWorkloads.md#performance-contract) and [Diagnostics comparison](../Architecture/CrossModule/PerformanceDiagnostics/Acceptance.md#comparison-and-regression-contract), including equal-N or justified tail treatment. Instrumented captures and optimized timing are separate runs. |
| Output join | Each row joins run/frame/scene/setting identity to original artifact and explicitly reports missing metrics; a merged chart must not conceal run-level tails or exclusions. |
| Stop and failure | Stale input, mismatched symbols, unconverged reference, changed settings, unsupported counter/tool or failed negative control prevents the claimed verdict. |

For example, an optimization table contains `run ID`, `API`, `baseline p95`, `candidate p95`, `absolute delta`, `relative delta`, `uncertainty`, `quality check` and `decision`. This describes required fields; no numbers or wins are fabricated here. Frame-time deltas are candidate minus baseline; percentage change divides that delta by the baseline, with units and sign stated.

## Admission And Remaining-Work Rules

The [Roadmap WIP/admission rule](Roadmap.md#dependency-and-work-in-progress-rule) selects work. An output here supplies the consumer need; its owning dossier freezes the defect-detecting check. A fillable card names known facts, unresolved decisions, owner, exit evidence and first permitted action. Reuse existing owners instead of creating framework placeholders for role keywords.

Completed implementation instructions are removed from their plans; enduring semantics, source evidence, valuable captures and decisions remain in their owners. Partial stages retain only their unfinished obligations and links to valid proof. [Feature Delivery Package](../Engineering/Workflow/Templates/FeatureDeliveryPackage.md#measurable-delivery-card) provides the reusable planning card and stage structure.

Unrun or missing hardware/software evidence stays unrun; excluded scope is explicitly excluded. Neither is accepted or “unsupported” without proof. Public claims include only closed evidence gates and can be narrower than this professional target.
