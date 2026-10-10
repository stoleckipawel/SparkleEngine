# Owned Neural Denoising Discovery

**Status:** populated discovery backlog; `NG-D0` is open and production implementation is not authorized by this page

**Admission:** after `REL-11` and the measured classical baseline `M3` in the [roadmap](../../../Strategy/Roadmap.md). Documentation research may proceed earlier. This is the selected `PGD-06/07` route in the [delivery catalog](../../../Strategy/FeatureDeliveryCatalog.md), not a generic ML platform.

**Product candidate:** one fixed diffuse-indirect denoising model, trained on owned/provenanced samples and executed as inspectable shaders through the existing Renderer. The [training contract](TrainingAndEvaluation.md), [runtime contract](ModelToKernelAndRuntimeInference.md) and [acceptance](Acceptance.md) own enduring semantics and proof requirements. Topology, operator set and numerical budgets remain decisions to freeze here before implementation.

## Known Facts And Boundaries

The [current dossier](README.md) distinguishes existing vendor reconstruction from absent owned data/model/lowering/runtime paths. The [Reference Path Tracer recovery plan](../../Modules/Engine/Renderer/Features/Lighting/ReferencePathTracer/Plan.md) records delivered source and unclosed oracle evidence; raw reference generation cannot be assumed reliable for training. [Graphics Workloads](../../../Acceptance/GraphicsWorkloads.md) already owns scenes, sample/benchmark policy and held-out generalization. [Shader System](../ShaderSystem/README.md) owns cook, reflection, ABI and published shader identity. Reuse these authorities.

The initial matrix targets available Windows/NVIDIA hardware with both D3D12 and Vulkan. Record the actual installed environment during discovery; this page does not certify it. Missing tuple evidence is blocked. AMD hardware and profiler work are excluded from this selected program. Optional cooperative-vector or vendor-specific kernels need their own pinned capability cell after the generic shader baseline, not a new mandatory dependency.

## Decisions Required For `NG-D0`

| Cell | Owner and decision to close | Artifact and defect-detecting exit |
| --- | --- | --- |
| `NG-D01` User output and comparison | Renderer/training owner: exact noisy diffuse-indirect input, denoised output, composition point, spatial MVP, classical/noisy baselines and supported content. | Product/feature matrix; one named output and equivalent baseline. Reject a mismatch in transport, resolution or lighting components before a quality verdict. |
| `NG-D02` Reference and radiometry | Reference/training owner: accepted estimator domain, raw linear units, demodulation/remodulation, exposure/normalization, sampling and uncertainty. | Pixel/tensor lineage and analytic fixtures; explicitly reject biased, shared-defect, post-processed, non-finite or unconverged targets. Reference ownership remains in its dossier. |
| `NG-D03` Rights and split identity | Data/Assets owner: licenses, frozen source/cook/camera/seed manifests, split granularity and retention. | Scene/sequence-separated train/validation/test map and leakage check. Final San Miguel test views remain held out; adjacent patches/frames are not independent split evidence. |
| `NG-D04` Model and training | Training owner: one fixed small topology/operator inventory, loss, optimizer, environment, seeds, repeat policy and reproducibility tolerance. | Reproducible recipe, overfit-one-batch diagnostic and classical/noisy comparisons. Freeze numeric acceptance before full training, not after selecting favorable results. |
| `NG-D05` Export and conformance | Tool/Shader owner: tensor/weight layout, padding/edge rules, operator equations, artifact identity, finite-value/range checks and FP32 reference. | Operator-by-operator and full-model fixture matrix; numeric absolute/relative tolerance, near-zero rule and reduction-order policy. An exported file alone never proves equivalent GPU inference. |
| `NG-D06` Runtime and lifecycle | Renderer/RHI/Assets owners: existing consumer/output seam, selection/fallback, immutable model generation, frame resources, capability checks, allocation/retirement and shutdown. | Producer/consumer/copy/deletion/hook ledger; trace one request through cook, binding, dispatch and retirement. No raw native pointers or model policy in generic hosts. |
| `NG-D07` Quality and cost | Workload owner: per-scene/reference-domain quality, temporal behavior even for a spatial model, tail latency, peak memory, startup and whole-frame effects. | Freeze numeric thresholds, units, masks, baselines, uncertainty/sample policy and win/loss/inconclusive rule; link the workload authority. Include bright/dark, disocclusion, motion, edges and distribution shift. |
| `NG-D08` Build and delivery | Build/Assets owners: dependency lock, notices, model/kernel manifests, supported API/profile matrix, optional payload erasure and clean failure. | Configuration/source/link/package membership card; missing/corrupt/incompatible artifact and capability-negative journeys. No training dependencies in runtime products. |
| `NG-D09` Scope and adoption | Feature owner and independent reviewer: architecture budget, one model, one consumer, personal/external ownership, baseline retained and non-author workflow. | Review signed to one dossier/source revision; `AC-NG-01..08` and `FM-NG-01..06` mapped to predeclared check cards; adoptable first-use route. |

Every cell records decision, source revision, evidence, unresolved owner and invalidation trigger. Unknown numeric tolerances or API/lifetime choices prevent `NG-D0` closure; the plan cannot choose them while coding. Prefer reducing topology/input scope to introducing a general runtime. No dataset capture should require new unbounded scene/material APIs.

## Primary Precedent

The [role and technical study](../../../Strategy/Research/PrincipalGraphicsRoles.md#technical-evidence-and-transfer-limits) records official PyTorch reproducibility, RTX neural shading, shader capability and Monte Carlo sources. These are precedent. When actually selecting a library, paper/model or compiler route, pin the original revision/specification, reproduce its relevant behavior and record rights here. No SDK, architecture or operator is adopted solely by mentioning that study.

### Reference Use In Remaining Discovery

The [source cards](../../../Strategy/Research/RenderingReferenceExamples.md) record immutable study locations and transfer limits. Select the relevant assumption and falsifier in each decision's artifact; these references do not select the model or close a cell.

| Discovery cells | Reference and what to inspect | Why / required local decision |
| --- | --- | --- |
| `NG-D01/02/06` | `NVR-03/04/15`: NRD guide encoding and tracer output decomposition. | Freeze the actual diffuse-indirect, raw-radiance, guide and remodulation contract. Wrong units/space/history must be detectable; neither an NRD integration nor a shared tracer qualifies as an independent neural oracle. |
| `NG-D03/04/08` | `NVR-11`: experiment inputs, snapshots and evaluation; existing reproducibility sources. | Pin the owned split/recipe/environment and exported model. Snapshot identity is not repeatable training; no NeRF product is admitted. |
| `NG-D05/07` | `NVR-10`: fused MLP activation storage/layout and hardware bounds; `NVR-18`: executed source correlation. | Derive the selected fixed operators, prove FP32 equivalence, then freeze one layout/fusion/precision hypothesis with quality and whole-frame checks. CUDA kernels are precedent, not portable shader code. |
| `NG-D06/09` | `NVR-01/13/20`: resource retirement, typed host/device examples and teaching exercises. | Reuse existing RHI/ShaderSystem identity/lifetime; one actual consumer and non-author explanation. No second RHI, OptiX adapter or training runtime. |

Use the [reference-use record](../../../Strategy/Research/RenderingReferenceExamples.md#delivery-reference-use-record) within `NG-D0`; record study-only versus actual reuse and rights. Accepted numeric/scope decisions remain authoritative until deliberately reopened.

## Ready-To-Use Discovery Prompt

```text
Execute only NG-D0 in Docs/Architecture/CrossModule/NeuralGraphics/Discovery.md. Research and planning only; do not change production code or train a final candidate. Apply AGENTS.md and the Feature Delivery Documentation Package.

Inspect current NeuralGraphics owners, ReferencePathTracer recovery/oracle status, GraphicsWorkloads, Renderer diffuse-indirect products and guides, ShaderSystem cook/reflection/binding, Assets publication, runtime selection/fallback, package membership and installed GPU/toolchain. Preserve the REL-11/M3 implementation prerequisite.

Use the selected NVR source cards from this discovery's reference-use table. Record exact source/file/assumption, local differences, rights and a falsifying check; do not import the source framework or substitute vendor integration for owned model work.

Close NG-D01..09 with exact sources, one fixed product/topology/operator inventory, tensor/radiometric/reference domain, licensed scene-separated splits, training reproducibility policy, numeric conformance/quality/cost thresholds, existing consumers, generation/retirement/failure protocol and hook/copy/deletion ledger. Record unresolved owners; no speculative generic APIs or dependency download. Revalidate primary sources before choosing a compiler/library/model route.

NON-NEGOTIABLE: classical and neural outputs compare the same product; references have accepted domain and uncertainty; San Miguel test identity is held out; numeric thresholds and repeat policy precede results; FP32 shader conformance precedes precision/vendor optimization; runtime policy has one feature owner. Quote each clause with evidence or BLOCKED. Stop on unproved oracle, rights, lifetime or unsupported installed tuple.

Update the existing training/runtime/acceptance contracts and Plan only from frozen decisions. Validate links/anchors/UTF-8/whitespace, traceability, architecture budget and git diff --check. Remove disposable probes. Handoff NG-D0 decisions, unresolved owners, accepted revision and first permitted stage; no readiness or executable acceptance claim.
```
