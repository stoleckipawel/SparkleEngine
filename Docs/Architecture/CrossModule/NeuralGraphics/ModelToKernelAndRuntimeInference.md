# Model To Kernel And Runtime Inference

**Status:** target capability dossier; no owned model lowering, generated kernel, or neural runtime was found

**Scope:** define the absent model-export, legalization, kernel-generation, runtime-execution, packaging, and completion-safe lifetime boundary

**Owner:** future model compiler/export tool plus Assets, Renderer, RHI, and Build/Packaging delivery owners

**Snapshot:** 2026-09-07; the live non-documentation source/build tree was searched for owned model export/lowering/kernel/inference-runtime paths; source evidence `S` only

**Current readiness:** **0/100** — target only; no model lowering, generated kernel, runtime activation, or packaged neural product was found. See [Current Feature Readiness](../../../Acceptance/CurrentReadiness.md#explicit-missing-or-not-yet-admitted-capabilities).

## At A Glance

| Stage | Owned decision | Required observable |
| --- | --- | --- |
| export/IR | accepted model/operator becomes a validated immutable schema | source/artifact hashes, shapes, operators, normalization, and compatibility |
| legalization/optimization | unsupported operators reject; fusion, layout, and precision choices remain inspectable | exact decisions plus numerical comparison to the accepted model |
| kernel production | generate or select backend/capability-specific kernels | code/kernel hash, target, workgroup/layout, resource/workspace contract |
| runtime activation | Renderer resolves requested/active neural or classical path | reason, inputs/output/history identity, and one complete generation |
| GPU execution | RHI materializes resources/pipeline/dispatch/barriers | attributable timing, memory, queue, and completion tokens |
| delivery | Build/Packaging manifests model/kernel/runtime bytes and licenses | clean product readiness or explicit unavailable state |

The target is not “compile a model somehow.” It is a reversible, inspectable chain in which every optimization can be compared numerically and every runtime artifact can be tied to the accepted model.

## Capability Boundary

This dossier owns the boundary from one accepted model/operator artifact to executable GPU work in the product. It is distinct from ordinary hand-written graphics shaders and from integrating a vendor provider. A credible model-to-kernel claim requires observable lowering decisions, validated numerical equivalence, hardware-aware optimization, one runtime ABI, and a product consumer.

## Intended Route

Accepted model/operator -> validated intermediate/export schema -> operator/legalization/fusion/layout/precision plan -> generated or selected kernels -> ShaderCompiler/build publication -> immutable runtime artifact -> Renderer typed inputs/outputs/history -> RHI dispatch/resources/barriers -> result validation -> completion-safe retirement.

## Contracts

- The artifact identity includes source model hash, schema/lowering version, target/backend/capability, precision/layout, kernel/code hash, binding ABI, and required runtime version.
- Unsupported operators, dynamic shapes, precision loss, capability mismatch, compilation failure, and validation mismatch fail before activation. Automatic fallback is allowed only when the requested/active distinction and reason are visible.
- Renderer owns semantic inputs/output and classical fallback. The lowering owner chooses legal kernels. RHI exposes mechanisms and capability truth without neural-policy vocabulary.
- Runtime scheduling declares resources, dependencies, queues, history, scratch/workspace, synchronization, and in-flight generation lifetime. Replacement retires only after every consuming queue completes.
- Evidence separates translation time, cold/warm materialization, CPU submission, GPU latency/throughput, memory/workspace, artifact size, and output quality.

## Design Decisions And Tradeoffs

| Decision | Benefit | Cost or risk |
| --- | --- | --- |
| One explicit intermediate/export schema | Lowering decisions and compatibility become reviewable | Schema ownership and invalidation must be maintained |
| Specialize kernels below stable model semantics | Hardware performance can improve without forking the feature oracle | Precision/layout/fusion can introduce numerical drift |
| Use typed Renderer inputs and output | Neural and classical paths remain comparable | Dynamic shapes and ad hoc tensors are deliberately constrained |
| Retire model/kernel generations by GPU completion | Reload cannot use freed artifacts or workspace | In-flight generations retain model/kernel/scratch memory |

## Current Classification

`NG-LOWER-01` model export/IR, `NG-LOWER-02` operator legalization/fusion, `NG-KERNEL-01` backend kernel generation/selection, `NG-RUNTIME-01` owned inference runtime, and `NG-RUNTIME-02` packaged product consumer are **Not found**. Existing HLSL/Slang compilation demonstrates graphics shader delivery, not ML model productization.

The `CASE-04` Model-to-Shader and `WL-06` runtime-inference workloads in [Graphics Workloads](../../../Acceptance/GraphicsWorkloads.md) are acceptance gates for this future route, not evidence it exists. See [Neural Graphics Acceptance](Acceptance.md).
