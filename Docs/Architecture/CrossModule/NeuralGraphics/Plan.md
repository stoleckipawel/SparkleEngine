# Owned Neural Denoising Delivery Plan

**Status:** remaining target work; conditional stage plan awaiting `NG-D0`, `REL-11` and `M3`

**Authority:** [Discovery](Discovery.md) freezes the slice; [Training And Evaluation](TrainingAndEvaluation.md), [Model To Kernel And Runtime Inference](ModelToKernelAndRuntimeInference.md) and [Acceptance](Acceptance.md) own contracts. [Roadmap](../../../Strategy/Roadmap.md) owns admission. No stage has candidate acceptance here.

This plan delivers one owned diffuse-indirect denoiser, with a useful classical fallback and both Windows graphics APIs. It deliberately has no calendar targets or time estimates. Shared engine repairs need an existing consumer and independently useful proof; do not hide a diagnostics/ML framework in this feature.

## Stage Contract

Apply [Change Integration](../../../Engineering/Workflow/ChangeIntegration.md), [Change Lifecycle](../../../Engineering/Workflow/ChangeLifecycle.md) and the [delivery package](../../../Engineering/Workflow/Templates/FeatureDeliveryPackage.md). Before each stage freeze the candidate, prerequisite revisions, file allowlist, producer/consumer/deletion and hook/copy ledgers, AC/FM/CHK mappings and smallest checks. Use existing shader/frame-graph/Assets/runtime/package ownership.

Each exit includes applicable formatting and logical-spacing checks from [Code Style](../../../Engineering/Foundations/CodeStyle.md), review of SRP/orchestration/naming, narrow builds/checks, directly affected docs and `git diff --check`. Run `architecture_boundary_check` when Renderer/RHI boundaries change. Disposable tests/probes are removed; retained artifacts go to the existing completion/workload report. No source/static check earns a numerical, GPU, performance, package or adoption verdict.

## Remaining Stages

| Stage | Prerequisite and bounded work | Exit, failure and deletion obligations |
| --- | --- | --- |
| `NG-S0` Freeze discovery | Run [NG-D0](Discovery.md); no production work. | All nine cells closed against a reviewed revision, with numeric checks and actual environment. Missing oracle, rights or lifetime remains explicit. |
| `NG-S1` Deterministic dataset | `NG-D0`, release admission, accepted reference output. One builder consumes existing immutable capture/export products; publish licensed scene-separated manifests. | `AC-NG-02`, `FM-NG-01`, relevant `CHK-NG-01`: repeated generation, leakage/partial/non-finite/reference-uncertainty negatives. Reuse schemas; delete replaced local generation routes. |
| `NG-S2` Owned training and model | Accepted dataset; one frozen topology and recipe. Train/repeat, compare noisy/classical baselines, ablate selected inputs/losses. | `AC-NG-03`, `CHK-NG-01`: overfit diagnostic, repeated numeric/metric tolerance, untouched held-out split, model card. Remove rejected model/recipe variants from supported routes; retain valuable experiment evidence. |
| `NG-S3` Export and FP32 conformance | Accepted model/semantics. Publish one minimal artifact and fixed operators; lower through existing ShaderSystem. | `AC-NG-04`, `FM-NG-02`, `CHK-NG-02`: operator/full-model PyTorch-export-GPU comparison on both APIs; shape/layout/padding/near-zero/extreme/invalid fixture checks. Delete replaced weight conversions; no runtime graph interpreter. |
| `NG-S4` Runtime vertical slice | FP32 conformance and runtime ledger. Typed Renderer inputs/output, frame-graph dependencies, model generation lifetime, explicit capability and classical fallback. | `AC-NG-05/07`, `FM-NG-03/05`, `CHK-NG-02/03`: actual dispatch/product identity, missing/corrupt/unsupported/allocation/compile/retirement/shutdown controls, honest requested/active state. No feature state in generic client/orchestrator. |
| `NG-S5` Measured kernel optimization | Correct runtime baseline. Independently evaluate FP16, layout, packing, dispatch, tiling/fusion and optional supported vector route, driven by measured limits. | `AC-NG-04/06`, `FM-NG-04`, `CHK-NG-02/03`: numeric drift and full-frame quality/latency/memory/observer results for each variant. Preserve FP32 baseline, delete rejected product variants. A negative result cannot become a speed claim. |
| `NG-S6` Evaluation, delivery and adoption | Supported runtime candidate. Held-out San Miguel/Bistro evaluation, failure gallery, manifests/licenses, clean-machine and independent reproduction. | `AC-NG-01..08`, `FM-NG-01..06`, `CHK-NG-01..04` complete for the declared matrix; `WL-07/08`. Corrupt package and unavailable tuple remain safe; exact model/kernel source joins. Publish only supported claims in the completion report. |

No unresolved architecture, threshold or tensor choice moves from discovery into a stage prompt. The first failed prerequisite stops dependent work, while independent documentation/research can continue. A scope revision returns affected cells to discovery and invalidates affected evidence.

## Ready-To-Use Stage Prompts

Use the stage contract above with every prompt; these are bounded entries, not permission to execute multiple stages.

```text
Execute NG-S1 only in Docs/Architecture/CrossModule/NeuralGraphics/Plan.md after NG-D0/REL-11/M3 and accepted reference prerequisites. Inspect existing capture/export and Assets publication before adding the one selected dataset builder. Preserve radiometry, split/reference/cook identity and license provenance. Validate repeat generation, scene leakage, partial/corrupt/non-finite and uncertain-reference rejection using the frozen NG-D checks. Delete replaced routes, apply code style, remove probes and hand off exact artifacts and AC-NG-02/CHK-NG-01 results. Stop on an unproved input or a new unledgered API. Do not train a model or add runtime code.
```

```text
Execute NG-S2 only after the accepted NG-S1 dataset. Use the frozen fixed topology, operators, losses, seeds/environment and repeat tolerance. Complete overfit-one-batch, noisy/classical baseline, repeat training and input/loss ablations without opening the held-out test for selection. Publish model/recipe identity and limitations; retain negative evidence and delete rejected supported variants. Apply stage style/validation controls and hand off AC-NG-03/CHK-NG-01. Stop on leakage, an unfrozen threshold or non-finite training. No GPU-runtime integration.
```

```text
Execute NG-S3 only after accepted model semantics. Inspect ShaderSystem cook/reflection/ABI and artifact publication. Implement only the frozen fixed export/operator and FP32 shader route. Verify operator and full-model equivalence to the frozen reference on D3D12/Vulkan, including edge/padding/invalid and numerical controls. Keep native lowering private and one weight identity; remove the replaced conversion route. Apply style and narrow owner checks; hand off AC-NG-04/CHK-NG-02 artifacts. Stop on numerical mismatch, guessed operator semantics or unowned lifetime. No FP16/vendor optimization.
```

```text
Execute NG-S4 only after NG-S3 conformance. Extend existing Renderer output/selection and frame-graph/Assets/RHI owners per the frozen consumer/copy/hook ledger. Prove input/output generation, declared dependencies, native retirement, capability and requested/active fallback behavior with failure and shutdown controls. No raw native API or model state escapes its owner; no UI-held feature truth. Remove replaced paths, run style/narrow build/boundary checks and hand off AC-NG-05/07. Stop on unowned lifetime or hidden fallback. No new scene/material API or generic tensor runtime.
```

```text
Execute NG-S5 only after accepted FP32 runtime. Profile the frozen workloads and test only justified precision/layout/dispatch/fusion or supported optional-vector candidates, one hypothesis per comparison. Freeze comparison controls before results; retain full-frame latency, peak memory, quality, numerical drift and observer context. Preserve the correct baseline, delete rejected product variants and report negative/inconclusive outcomes honestly. Apply style/narrow owner checks; hand off AC-NG-04/06 and exact tuples. Stop on drift, hidden quality/settings changes or threshold edits. No mandatory new vendor SDK.
```

```text
Execute NG-S6 only after the supported runtime/optimization candidate. Close the existing AC-NG/FM-NG/CHK-NG matrix and WL-07/08 using held-out views, declared reference uncertainty, optimized observer controls, package hashes/licenses/source joins and non-author reproduction. Reuse only valid candidate-bound proof. Apply style/docs checks; remove probes and obsolete routes. Record results in the existing completion/workload report, never in a new readiness authority. Stop on missing adopter, symbol/model mismatch, unsafe failure or incomplete package; preserve accepted individual cells. No new topology or unrelated feature.
```

Every production prompt inherits this non-negotiable exit: one identity and owner per data/runtime boundary; accepted reference and frozen numerical rules; no held-out leakage; correct FP32 route before optimizations; actual GPU/native results distinguished from source; complete controlled failures; no unledgered hooks or duplicate mechanisms. Quote each clause with retained proof or `BLOCKED` at handoff.
