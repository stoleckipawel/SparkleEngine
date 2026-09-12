# Renderer Lighting Closure Plan

**Status:** cross-feature release sequence for direct/indirect closure and the `PTD-00` handoff; feature-local implementation order lives in the linked Direct and Indirect plans and no plan is evidence

**Families:** `FCR-REN-06`, `FCR-REN-07`, `FCR-REN-08`

**Current readiness:** section projection **37/100**; direct/indirect source routes exist, the Reference Path Tracer family is **20/100**, and all three remain `Blocked`

**Responsibility:** sequence direct and indirect lighting closure and enforce the discovery gate before Reference Path Tracer implementation planning

**Parent:** [First Release Renderer Plans](README.md)

**Architecture:** [Lighting](../Features/Lighting/README.md), [Direct Lighting](../Features/Lighting/DirectLighting/README.md), [Indirect Lighting](../Features/Lighting/IndirectLighting/README.md), [Volumetric Lighting](../Features/Lighting/VolumetricLighting/README.md), and [Reference Path Tracer](../Features/Lighting/ReferencePathTracer/README.md)

## Plan At A Glance

```mermaid
flowchart LR
    L0[LGT-0<br/>shared units and oracle contract] --> D0[DIR-D0 / IND-D0<br/>feature discovery]
    D0 --> L1[LGT-1<br/>direct plan]
    L1 --> L2[LGT-2<br/>indirect plan]
    L2 --> D[PTD-00<br/>discovery gate]
    D -->|PASS| P[PTD-01<br/>freeze conditional plan]
    D -->|BLOCKED| S[stop or re-scope release]
    P --> L4[LGT-4<br/>three-family closure]
```

| Phase | Primary family | Current boundary |
| --- | --- | --- |
| `LGT-0` | all three | source routes exist, but shared units/material/light/reference contract must be reconciled |
| `LGT-1` | `FCR-REN-06` | direct source reservoir exists; `DIR-D0` and [the feature plan](../Features/Lighting/DirectLighting/Plan.md) own its correctness-led clean break |
| `LGT-2` | `FCR-REN-07` | indirect seed-replay source exists; `IND-D0` and [the feature plan](../Features/Lighting/IndirectLighting/Plan.md) own its path/shift/GRIS replacement |
| `LGT-3` | `FCR-REN-08`, discovery only | `PTD-00` is unpassed; current path cannot be called an unbiased oracle |
| `PTD-01` | `FCR-REN-08`, implementation | conditional plan exists; it cannot be accepted or advance past Stage 0 until discovery freezes its goals |
| `LGT-4` | all three | candidate comparison/adoption reports absent |

> [!CAUTION]
> `LGT-3` is a hard implementation stop. The [discovery contract](../Features/Lighting/ReferencePathTracer/Discovery.md) deliberately requires `PTD-00` to pass before the conditional `PTD-01` plan can be frozen or advance beyond Stage 0. Provisional transport choices and prompts remain non-authoritative until discovery tests and accepts them.

NVIDIA's [RTXDI integration guide](https://github.com/NVIDIA-RTX/RTXDI/blob/main/Doc/Integration.md) is a primary reference for the boundary where the application owns scene, materials, GBuffer, rays, and API integration while the SDK supplies reservoir algorithms. [RTX Path Tracing](https://github.com/NVIDIA-RTX/RTXPT) and the repository's [completion study](../Features/Lighting/ReferencePathTracer/Research.md) are precedents and research inputs, never local proof.

## `LGT-0` — Freeze Units, Estimators, And Comparison Domain

**Goal:** establish one testable material/light/sky/unit, sampling identity, raw-output, and comparison contract for all three families.

**Non-goals:** choosing the Reference Path Tracer transport scope before discovery, tuning for release screenshots, or forcing direct/indirect/reference histories into one lifetime.

**Required work:** reconcile four light types/limits, material lobes, units/spaces, sky/environment boundary, surface/ray spawn, PDFs/weights/reservoir validity, deterministic sample identity, output lobe channels, composite, history/reset, debug separation, backends/routes, quality/cost/memory matrices, and feature `AC/FM/CHK`.

**Failure modes:** unit mismatch hidden by exposure; invalid PDF/weight; NaN/Inf clamped before evidence; direct energy double-counted; sky semantics differ; post-tonemapped image used as raw oracle; stochastic comparison lacks seed/sample identity.

**Phase exit criteria:** analytic fixtures and raw observables exist for every currently admitted direct/indirect cell; `FCR-REN-08` remains explicitly blocked on `PTD-00`; no undocumented transport claim enters implementation.

**Ready-to-use prompt:**

```text
Execute LGT-0 without transport implementation. Create ITER-REN-LGT-00 mapped to FCR-REN-06/07/08 and current AC/FM/CHK. Inspect cooked/world light and material semantics, Renderer scene/GBuffer, direct/indirect shaders and reservoirs, sky/composite, ray spawn, temporal inputs, debug outputs, selectors, RHI routes, and package membership. Record units, spaces, lobe ownership, PDFs/weights, sample identity, raw outputs, reset/invalidation, backend/content matrix, analytic fixtures, and quality/time/memory oracles. Reconcile Architecture wording only. Keep FCR-REN-08 blocked and stop if PTD-00 assumptions would be invented.
```

## `LGT-1` — Direct Lighting

**Goal:** sequence the accepted [Direct Lighting plan](../Features/Lighting/DirectLighting/Plan.md) to close `FCR-REN-06` for all admitted analytic light kinds, material lobes, reservoir reuse, visibility, reconstruction, and many-light behavior.

**Non-goals:** expanding light types, replacing the BRDF, hiding bias/leaks with post effects, or separate inline/RGS lighting implementations.

**Required work:** prove light units and limits, BRDF/lobe outputs, candidate generation/PDF/weight/reservoir validity, temporal/spatial reuse if admitted, shadow visibility/bias, alpha/two-sided/subsurface scope, inline/native parity, invalid/missing data behavior, debug separability, and cost; use analytic single-light/material/occluder fixtures before maps.

**Failure modes:** zero/negative/non-finite weight; stale reservoir after identity/reset; self-intersection/acne or light leak; unsupported traversal hidden; light beyond limit corrupts buffer; one lobe receives duplicate/missing energy.

**Phase exit criteria:** `DIR-D0` is accepted and every required stage/criterion in the feature plan passes or produces an explicit blocker in `FCR-REN-06`.

**Ready-to-use prompt:**

```text
Execute exactly one authorized stage from Docs/Architecture/Modules/Engine/Renderer/Features/Lighting/DirectLighting/Plan.md. Require accepted DIR-D0, the stage prerequisites, and the feature plan's universal execution contract. Keep LGT-0 shared units/oracles and FCR-REN-06 identity fixed; do not collapse or reorder stages, tune the current reservoir before its analytic baseline, or preserve a superseded estimator. Return the stage's exact checks, artifacts, hook/deletion ledger, unrun cells, and blocker/pass disposition before another stage begins.
```

## `LGT-2` — Indirect Lighting

**Goal:** sequence the accepted [Indirect Lighting plan](../Features/Lighting/IndirectLighting/Plan.md) to close `FCR-REN-07` for the explicitly admitted path, shift, GRIS, reuse, material, sky, temporal, reconstruction, and failure domain.

**Non-goals:** calling the current Reference Path Tracer route ground truth, increasing bounces/features without acceptance need, or tuning away bias without identifying it.

**Required work:** reconcile candidate/sample generation, PDFs/weights/reservoir math, secondary ray/material/sky evaluation, bounce scope, direct/indirect separation, temporal/spatial reuse, motion/disocclusion/reset, firefly/non-finite policy, deterministic seeds, inline/RGS/backend equivalence, raw lobe outputs, artifact gallery, and quality/time/memory frontier.

**Failure modes:** zero PDF or exploding weight; reuse crosses view/scene generation; disocclusion ghosts; sky double-counts; firefly clamp biases silently; NaN/Inf enters history; reset leaves stale energy; route/backend diverges statistically.

**Phase exit criteria:** `IND-D0` is accepted and every required stage/criterion in the feature plan passes or produces an explicit blocker in `FCR-REN-07`.

**Ready-to-use prompt:**

```text
Execute exactly one authorized stage from Docs/Architecture/Modules/Engine/Renderer/Features/Lighting/IndirectLighting/Plan.md. Require accepted IND-D0, its stage prerequisites, and the feature plan's universal execution contract. Keep LGT-0 shared units/oracles and FCR-REN-07 identity fixed; do not label the current seed replay ReSTIR GI, skip the explicit path/shift/GRIS clean break, expand path depth early, or use FCR-REN-08 before its accepted oracle gate. Return exact checks, artifacts, hook/deletion ledger, unrun cells, and blocker/pass disposition before another stage begins.
```

## `LGT-3` — Execute `PTD-00` Discovery

**Goal:** answer the exact research questions needed to define an honest, achievable `FCR-REN-08` implementation plan.

**Non-goals:** modifying the path tracer, asserting unbiasedness, importing RTXPT/Falcor architecture, or drafting detailed implementation phases before the decision.

**Required work:** execute every discovery item, risk, failure mode, check, and evidence package named by the [binding `PTD-00` contract](../Features/Lighting/ReferencePathTracer/Discovery.md); use the [research study](../Features/Lighting/ReferencePathTracer/Research.md) and pinned primary sources; distinguish verified local source, observed behavior, mathematical requirement, precedent, unknown, and release decision.

**Failure modes:** source inspection reported as runtime proof; external renderer result attributed locally; transport terms undefined; unsupported material/light silently excluded; numerical tolerance chosen without oracle; gate passes with missing evidence.

**Phase exit criteria:** the Architecture gate records `PASS` with all required development outputs, or records `BLOCKED`/re-scope. Only an immutable repository-owner-accepted `PTD-00-R1 PASS` freezes `PTD-01` and authorizes development Stages 1 through 9. `REL-03`, accepted release maps, and named support machines remain mandatory before Stage 10, `FCR-REN-08` release closure, and release-map oracle adoption.

**Ready-to-use prompt:**

```text
Execute PTD-00 exactly as Docs/Architecture/Modules/Engine/Renderer/Features/Lighting/ReferencePathTracer/Discovery.md and Stage 0 of its Plan.md specify. Create its iteration record at the current revision and preserve source/observed/reference distinctions. Inspect the complete local transport, material, light, ray, accumulation, export, reset, backend, package, and evidence routes; run only the discovery checks the contract requires. Compare against the pinned NVIDIA, AMD, and neutral mathematical sources for explicit questions, recording revisions and transfer boundaries. Produce the required evidence package and binary gate decision. Do not change implementation, freeze the plan, or start Stage 1 on BLOCKED/incomplete results.
```

## Conditional `PTD-01` Handoff — Freeze The Reference Path Tracer Plan From Accepted Facts

**Goal:** after `PTD-00 PASS`, create the dedicated path-tracer plan whose phases close every included `RPT-FS-*`, `AC-RPT-*`, `FM-RPT-*`, and `CHK-RPT-*` without invented requirements.

**Non-goals:** this document serving as that plan, implementing during plan authoring, or retaining rejected discovery assumptions.

**Failure modes:** prompt runs before gate pass; plan omits an accepted transport row; vendor precedent becomes local goal; acceptance truth is duplicated; phase cannot be independently checked.

**Phase exit criteria:** `Docs/Architecture/Modules/Engine/Renderer/Features/Lighting/ReferencePathTracer/Plan.md` exists only after gate authorization, is linked from plan indexes, maps every accepted discovery output and acceptance row, and contains per-phase goals/non-goals/failures/exit criteria/prompts.

**Ready-to-use prompt:**

```text
Run this prompt only if PTD-00 has an explicit PASS and complete accepted evidence package. Reconcile the conditional PTD-01 at Docs/Architecture/Modules/Engine/Renderer/Features/Lighting/ReferencePathTracer/Plan.md to that exact revision. Replace every provisional current-state, transport, algorithm, numerical oracle/tolerance, material/light/backend/package, dependency, estimate, and stop-condition choice with accepted PTD-00 output. Verify every included RPT-FS, AC-RPT, FM-RPT, and CHK-RPT maps to the correct owner-sized vertical phase and that every prompt preserves its non-goals, controlled failures, exit checks, escalation, and reference-transfer boundary. Do not edit production code, copy acceptance authority, or add vendor architecture by analogy. Validate index links and no-orphan coverage before authorizing Stage 1.
```

## `LGT-4` — Lighting Candidate Closure

**Goal:** after `FCR-REN-08` implementation through the authorized `PTD-01`, compare direct/indirect results with the accepted Reference Path Tracer and close all three families for one candidate.

**Non-goals:** treating one attractive image as convergence, allowing the reference to share the same defect without independent checks, or skipping packaged/backend routes.

**Failure modes:** reference/candidate share unverified input; accumulation not converged; exposure/tone confounds raw result; one backend/provider differs; artifact gallery is cherry-picked; path-tracer evidence predates lighting changes.

**Phase exit criteria:** direct, indirect, and Reference Path Tracer reports satisfy their complete Architecture contracts; raw/reference, convergence, analytic/independent, failure, backend, package, quality/time/memory, and release-map evidence share one candidate identity.

**Ready-to-use prompt:**

```text
Execute LGT-4 only after PTD-01 implementation has a candidate-bound result. Freeze the candidate and reconcile FCR-REN-06/07/08 plus all RPT criteria/checks. Run missing analytic, deterministic, convergence, raw-lobe, direct/indirect/reference, failure, history/reset, inline/RGS, D3D12/Vulkan, packaged, and release-map comparisons. Keep sample identity, settings, input assets, camera, output domain, tolerances/confidence, and independent references explicit. Any lighting/material/ray/shader change invalidates affected evidence. File exact decisions, limitations, and blockers; never call the reference unbiased beyond its accepted scope.
```
