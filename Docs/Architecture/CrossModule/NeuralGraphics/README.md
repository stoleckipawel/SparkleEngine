# Neural Graphics Architecture

**Status:** target cross-module feature family; current vendor reconstruction inference is not an owned neural-graphics implementation

**Owner:** cross-module contract spanning future model/tool ownership, Assets publication, Renderer integration, RHI execution, Showcase workloads, and Build/Packaging delivery

**Snapshot:** 2026-09-07; Engine, Tools, Projects, CMake, and root build membership were searched for owned training, dataset, model-lowering, and neural-runtime paths with no match; source evidence `S` only

**Strategy sources:** [`PGE-03`, `PGE-04`, `PGE-11`, `PGE-12`](../../../Strategy/Requirements.md), [Executive Summary](../../../Strategy/ExecutiveSummary.md), [Gap Assessment](../../../Strategy/Assessments/GapAssessment.md), and [Graphics Workloads](../../../Acceptance/GraphicsWorkloads.md)

## Capability Identity

| ID family | Capability | Current state |
| --- | --- | --- |
| `NG-TRAIN-*` | dataset/provenance, owned model/operator, and reproducible training | Not found |
| `NG-EVAL-*` | independent quality, temporal, robustness, and cost evaluation | Not found |
| `NG-LOWER-*` | model export, intermediate representation, legalization, fusion, layout, and precision | Not found |
| `NG-KERNEL-*` | backend kernel generation or selection and numerical validation | Not found |
| `NG-RUNTIME-*` | owned Renderer/RHI inference, fallback, lifetime, and packaged product route | Not found |

## Feature Family

| Dossier | Owns | Current state |
| --- | --- | --- |
| [Training And Evaluation](TrainingAndEvaluation.md) | problem definition, dataset/provenance, reference target, model/operator, training, evaluation, reproducible artifact | Not found |
| [Model To Kernel And Runtime Inference](ModelToKernelAndRuntimeInference.md) | export/lowering, kernel selection/optimization, runtime ABI, scheduling, fallback, profiling, delivery | Not found |
| [Acceptance](Acceptance.md) | binary feature criteria, failures, checks, and evidence boundary | Contract only |

The current NVIDIA DLSS Super Resolution and Ray Reconstruction providers are documented under [Image Reconstruction And Upscaling](../../Modules/Engine/Renderer/Features/PostProcessing/ReconstructionAndGeneration/ImageReconstructionAndUpscaling.md). They are external vendor inference integrations. They do not satisfy owned dataset, training, model, compiler/lowering, kernel, or evaluation claims.

## Intended Vertical Slice

Frozen representative scene data and references -> deterministic dataset builder -> owned model/operator and training recipe -> evaluated versioned artifact -> explicit export/lowering and GPU-kernel plan -> Renderer feature contract and classical fallback -> RHI resources/dispatch/synchronization -> packaged artifact -> Bistro/San Miguel quality, latency, memory, and failure evidence.

No arrow in this route is implemented merely because this target dossier exists. The feature remains absent until source/build membership and a reachable product consumer prove otherwise.

## Ownership Rules

- Training data and evaluation artifacts have one provenance/version owner; runtime code does not silently generate or mutate them.
- Model/operator semantics are independent of backend kernels. Backend lowering may specialize execution without forking the feature definition or oracle.
- Renderer owns requested/active feature selection, inputs, output/history identity, fallback, and user-facing diagnostics. RHI owns neutral GPU mechanisms and backend lowering.
- Build/Packaging owns immutable model/kernel delivery and redistribution. Showcase owns only representative workload selection, not feature truth.
- Classical and neural paths share one observable output contract so quality/performance comparisons are meaningful.
