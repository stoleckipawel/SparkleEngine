# Direct Lighting Staged Implementation Plan

**Status:** implementation-ready sequence, conditionally blocked by `DIR-D0` and repository release admission; stages are not implementation evidence

**Responsibility:** own dependency order, stage scope, prerequisites, estimates, clean-break work, non-goals, exit gates, prompts, and stop rules

**Authority boundary:** this plan consumes accepted Discovery/Semantics/Architecture/User Experience and cannot change them; code/build proves implementation and `FCR-REN-06` alone owns candidate results

**Plan revision:** `DIR-PLAN-02`, deepened 2026-09-13 from repository revision `8b650c7450f8a59fb3bcc18edbb4d217a7b11ed5`

## Outcome

Deliver a correct, diagnosable, portable direct-lighting path that scales from exact small-light fixtures to dense dynamic lighting through canonical reservoir reuse and bounded reconstruction. Replace the visually unreliable current path by evidence, not by accumulating alternate implementations.

## Delivery Priority

1. reproduce and localize raw failure;
2. freeze light/BRDF/PDF semantics and build an exhaustive baseline;
3. replace the reservoir representation and initial estimator;
4. add temporal/spatial reuse with stable identity and bias control;
5. prove visibility frontends and failure behavior;
6. add portable reconstruction and optional RR;
7. admit emissive/environment/ReGIR only from measured need;
8. close paired-backend/workload evidence and `FCR-REN-06`.

## Gate And Dependency Graph

```text
release/admission gate ----+
DIR-D0 --------------------+--> DIR-1 --> DIR-2 --> DIR-3 --> DIR-4
RPT accepted direct AOV ---+       |         |                  |
analytic CPU oracle -------+-------+---------+------------------+
                                                             DIR-5 --> DIR-6 --> DIR-7 --> DIR-8
```

The Reference Path Tracer is helpful for scene-level comparison only after its own evidence gate. `DIR-1/2` do not wait for it because analytic and exhaustive tests are deliberately independent.

## Planning Envelope

| Item | Estimate after gate | Principal uncertainty |
| --- | ---: | --- |
| Stage 0 discovery | 4–8 engineering days | reproduction, oracle readiness, product admission |
| Stages 1–2 correctness foundation | 8–15 days | current BRDF/light semantic defects and test infrastructure |
| Stages 3–4 reservoir/reuse replacement | 12–25 days | correction math, packing, motion/light translation |
| Stage 5 visibility parity | 5–12 days | Pipeline readiness, alpha/two-sided backend differences |
| Stage 6 reconstruction | 8–18 days | chosen baseline and existing RR integration state |
| Stage 7 scale expansion | 0–25 days per admitted increment | emissive inventory/ReGIR/product scope |
| Stage 8 evidence/adoption | 6–15 days | backend/hardware/workload availability |

These are planning ranges, not commitments. `DIR-D0` replaces them with owner-reviewed estimates after experiments. Every stage is one reviewable production slice; split a stage if its hook ledger or validation cannot be reviewed coherently.

## Universal Execution Contract

Every stage prompt inherits these rules:

- inspect revision/status, applicable `AGENTS.md`, owner docs, source consumers/producers/lifetime/build membership, and concurrent changes before editing;
- follow Change Integration, Change Lifecycle, Renderer, Module Ownership, Data And Memory, and claim-driven validation rules;
- keep mechanism/state in the feature capsule and ledger every external feature-named hook;
- extend current owners directly; no legacy path, alias, migration reader, duplicate scene/material/light record, generic framework, forwarding wrapper, or diagnostic subsystem;
- update code, headers, shaders, typed bindings, registrations/cooking, build/package membership, checks, and directly affected docs together;
- preserve unrelated dirty work and do not modify Reference Path Tracer files unless the stage explicitly owns a reviewed cross-feature contract change;
- select the cheapest check that can falsify the stage claim; never infer build/runtime/GPU/visual/performance evidence from static checks;
- stop when a frozen semantic is missing, a required owner is concurrently changed, or the proposed work exceeds the stage's admission.

## Stage Delivery Contract Matrix

This matrix is part of each stage, not optional guidance. “Production delta” names the responsibility to change; Stage 0 must replace it with exact files and build/generated members before authorizing that stage.

| Stage | Prerequisites | Required production delta and deletion | Explicit non-goals | Retained deliverables | Smallest stage falsifier |
| --- | --- | --- | --- | --- | --- |
| 0 | current revision/status/source trace; release owner available; raw capture route or bounded probe | documentation/fixtures/probes only; no estimator behavior change; delete temporary probes before handoff | tuning constants, rewriting shaders, enabling an SDK | accepted decision register, raw-failure manifest, oracle/threshold/profile table, exact hook/deletion/build map, revised estimate, review record | any `DIR-D0-*` choice, threshold, owner, rights action, or first-slice file remains unresolved |
| 1 | accepted `DIR-D0-01/02/03/04/11/14`; admitted work; fixture/reference rights | add independent CPU/closed-form oracle and bounded exhaustive GPU product through existing light/material/ray/frame routes; remove only proved obsolete temporary diagnostic code | reservoir change, temporal reuse, denoising, new lobes/providers | analytic vectors, exhaustive raw lobes, per-light breakdown, invalid/capacity results, shared-code analysis, commands/configuration | analytic or exhaustive result misses a frozen threshold, or oracle depends entirely on questioned production code |
| 2 | Stage 1 pass; frozen equations/units/lobes/shapes; exact semantic call sites | establish one shared focused evaluation/sampling/PDF semantic owner and update all production/reference consumers; delete duplicated/replaced light/BRDF/PDF rules | reservoir/reuse, emissive inventory, ReGIR, reconstruction, new material model | equation-to-code review, CPU/shader parity, furnace/shape/PDF artifacts, source/build/deletion map | any consumer retains a competing formula or sampling PDF disagrees with evaluation/support |
| 3 | Stage 2 pass; accepted target/proposals/record/precision; frozen old-state deletion list | replace initial reservoir/sample representation, pass bindings, shaders and registrations atomically; retain exhaustive oracle; delete old representation/readers/writers | temporal/spatial reuse, visibility caching, reconstruction, advanced proposals not admitted | logical/packed layouts, round-trip/max/finite tests, discrete/statistical results, fresh-only raw/reference captures, stale-symbol search | old and new representations coexist, a contributing light has zero support, or mean/selection statistics fail |
| 4 | Stage 3 pass; accepted correction mode, light translation, history/compatibility/random layout and budgets | add immutable previous/current per-View reservoirs/surface metadata, temporal/spatial passes, transactional publication/retirement; delete obsolete coarse history path | reconstruction, new light classes/providers, pass fusion or large-kernel research | mutation matrix, decoded histories/rejections, diversity/autocorrelation/convergence curves, long-run/dual-view/resize artifacts, memory overlap | wrong-light/stale/cross-view reuse, non-finite publication, or reuse increases error beyond frozen bound |
| 5 | Stage 4 pass; accepted Inline/Pipeline support matrix and ray semantics; backend fixtures | reconcile provider lowering/bindings/program/SBT/TLAS/material access to one finite visibility contract; delete divergent duplicate semantics | screen trace, VSM/shadow-map system, OMM, generic provider framework | paired raw visibility/hit artifacts, alpha/two-sided/self-hit/segment cases, capability-fault results, architecture boundary result | providers disagree outside accepted tolerance or strict unavailability schedules partial work |
| 6 | Stages 4/5 pass; frozen portable reconstruction signal/history/UX and optional-provider policy | add portable reconstruction through existing provider owner, exact guides/confidence/history/status and optional RR adapter; delete temporary filter experiments | feeding filtered values to estimator, tuning around raw bias, learned sole path | raw/reconstructed sequences, guide manifests, lag/detail/error/time/memory, provider fault/bypass/reload results, first-use/automation UX | filter hides failed raw estimator, stale guides survive, or portable baseline is unavailable on a mandatory profile |
| 7 | Stage 6 pass; exactly one `DIR-D0` expansion admitted from a reproduced failing workload | add only environment candidates, emissive triangles, ReGIR, or another specifically admitted increment through existing scene/content owners; delete failed experiment if gate misses | bundling expansions, shadow architecture, new material lobes, “unlimited” claim | source/provenance record, distribution/update/generation tests, equal-time A/B, mutation/retirement, hook/removal ledger | no frozen equal-time benefit, duplicated light/content ownership, unsupported zero-probability source, or budget breach |
| 8 | all included stages pass; exact candidate/configuration/hardware/content/reference ready; `FCR-REN-06` owner available | no feature redesign; only candidate-bound fixes explicitly re-admitted as a new candidate; update current docs/build/package/generated surfaces and remove stale prototype names | opportunistic tuning/refactor, borrowing proof across backends/providers, repairing unrelated failures | complete FCR evidence bundle, UX/automation/accessibility, workload/backend/native-validation, time/memory distributions, enclosure/removal and package results | any included AC/FM/CHK lacks observed candidate-bound disposition or any production claim exceeds evidence |

## Required Stage Handoff

Every stage retains:

1. start/end revision and dirty-boundary statement;
2. accepted prerequisite decision IDs and links to their evidence;
3. changed and deleted files grouped by responsibility, including build/generated/package membership;
4. complete outside-feature hook ledger and bounded-removal result;
5. semantic-rule-to-code/check mapping and public-surface delta;
6. exact commands, configurations, raw artifacts, results and cleanup;
7. failed/unavailable/unrun build, runtime, GPU, backend, visual, performance, package, accessibility and clean-machine checks;
8. remaining risks/limitations and a binary statement that the next stage is `Authorized` or `Blocked`.

No stage hands off a partial history, compatibility mode, dormant experiment, unowned diagnostic, or result detached from candidate identity.

## Copy-Ready Prompt Contract

Every stage prompt is the stage-specific quotation plus the following mandatory tail; copy both together:

> NON-NEGOTIABLE: verify the stage's matrix prerequisites and accepted Discovery/Semantics/Architecture/User Experience contracts before editing. Keep all mechanism and state in the frozen feature home; generic owners may only perform the ledgered selection, publication, composition, capability, content, or build hook. Make the listed clean-break deletions in the same change, preserve the independent raw/oracle path, and do not add an unratified algorithm, unit, identity, lifetime, fallback, public control, threshold, budget, provider, compatibility path, wrapper, or diagnostic subsystem.
>
> STOP: report `BLOCKED` before production mutation if a prerequisite/equation/oracle/threshold/owner/rights/capability is absent, current evidence contradicts the contract, concurrent work overlaps an owned path, or the diff exceeds the stage. Handoff must quote every stage deliverable and exit condition with candidate-bound proof, list all changed/deleted hooks and exact checks/results/artifacts, state cleanup and every unrun check, and explicitly say whether the next stage is authorized.

## Stage 0 — Close Discovery And Authorize The First Slice

### Objective

Turn the user's visual-failure report and this research package into frozen, reviewable decisions and a bounded production change.

### Work

1. Capture the current failure in raw direct lobes, selected light/sample, reservoir state, visibility, and final preview with exact candidate identity.
2. Execute `DIR-X-01` through `DIR-X-09`, resolve `DIR-D0-01` through `DIR-D0-14`, and fill the evidence package.
3. Freeze formulas, units, light identities, target/bias mode, compatibility, visibility, denoiser, quality profiles, artifacts, thresholds, budgets, hook ledger, and clean-break deletions.
4. Reconcile first-release admission and assign owners/revised estimates.
5. Obtain independent review of the gate; retain `Open` or `Blocked` for every unresolved cell.

### Exit Gate

- `DIR-D0` is explicitly accepted with immutable artifacts and no unresolved Stage-1 semantic.
- The release owner authorizes Stage 1.
- The reproduction distinguishes estimator, shading, visibility, reconstruction, or still-unknown failure; a screenshot alone does not pass.

### Ready-To-Use Prompt

> Execute Direct Lighting Stage 0 from `DirectLighting/Plan.md`. Treat the task as discovery and evidence, not a renderer rewrite. Run `DIR-X-01` through `DIR-X-09`, reproduce the reported visual failure with raw direct lobe, reservoir, selected-light/PDF, and visibility artifacts; close every `DIR-D0-*` decision; freeze thresholds and budgets before candidate observation; produce the exact hook/deletion ledger and revised estimates; and request independent gate review. Do not change production estimator behavior. Preserve unrelated work and report every unrun check.

## Stage 1 — Establish Analytic And Exhaustive Truth

### Objective

Create the smallest independent foundation that can identify whether light units/geometry, BRDF lobes, selection PDFs, or visibility produce the wrong raw value.

### Work

1. Add CPU/closed-form light and BRDF hand-case checks using frozen `DIR-D0-02/03` semantics.
2. Add a bounded exhaustive GPU direct resolve for test/developer fixtures using the production GBuffer/light/material boundary and the same raw lobe outputs.
3. Cover one/exact-capacity/overflow, inverse-square, cone edges, rectangle Jacobian/sidedness, roughness/metallic/F0, diffuse/subsurface allocation, black/zero/degenerate/non-finite inputs.
4. Add raw artifact decode with existing capture/test infrastructure; temporary probes must be removed before submission.
5. Record which current defects are proved and which remain hypotheses.

### Exit Gate

- `CHK-DIR-01` passes for the frozen matrix.
- Exhaustive GPU output agrees with the independent oracle at predeclared tolerances.
- No reservoir, temporal reuse, or denoising participates in the claim.
- Exact new production/test file and integration-hook ledger is reviewed.

### Ready-To-Use Prompt

> Implement Direct Lighting Stage 1 only. Build the accepted analytic CPU/closed-form cases and one bounded exhaustive GPU direct-lighting baseline over the production surface/light boundary. Produce raw `DirectDiffuse`, `DirectSpecular`, and `DirectSubsurface` evidence without reservoir reuse, reconstruction, exposure, or tone mapping. Cover units, light geometry/PDFs, BRDF energy allocation, capacities, and invalid inputs. Do not tune or replace ReSTIR yet. Keep tests local-only if they require new test-only files, and do not submit temporary probes.

## Stage 2 — Freeze One Shared Light And Surface Semantic Kernel

### Objective

Make production direct evaluation and candidate evaluation consume one proved semantic implementation without turning shared math into a generic framework.

### Work

1. Reconcile GameFramework descriptors, prepared GPU records, shader fields, and Reference Path Tracer semantic contract for units and shapes.
2. Implement the ratified BRDF/lobe allocation, shape sampling, selection/conditional PDFs, robust support checks, and CPU equivalents.
3. Update all producers/consumers in one clean break; remove superseded dormant alternatives if they are neither compiled choices nor required reference helpers.
4. Validate shader parameter/layout and cooking dependency closure.
5. Re-run Stage 1 cases and capture equation-to-code mapping.

### Exit Gate

- `AC-DIR-01/02/03` and `FM-DIR-01` are satisfied for this stage's candidate.
- Evaluation and sampling PDF tests agree over deterministic and statistical cases.
- No duplicated unit conversion or BRDF/light representation remains.

### Ready-To-Use Prompt

> Implement Direct Lighting Stage 2 from the accepted `DIR-D0` equations. Establish one shared, focused light/BRDF semantic kernel for exhaustive resolve and reservoir candidate evaluation; update descriptors/preparation/shaders/callers together; and delete the replaced internal semantics. Prove evaluation/PDF support, lobe energy allocation, shape Jacobians, and finite handling with Stage 1 tests. Do not add new material lobes, emissive meshes, ReGIR, denoising, or a shadow-map framework.

## Stage 3 — Replace Initial Reservoir State And Sampling

### Objective

Land a canonical, testable initial-candidate reservoir with stable integer identity and exact source probabilities, without temporal or spatial reuse.

### Work

1. Introduce the ratified packed/unpacked reservoir under the feature owner; record integer light/sample/frame identity without binary32 round trips.
2. Implement deterministic stream update, bounded `M`, target support, final normalization, and finite/invariant response.
3. Implement uniform oracle and power-weighted initial distributions plus frozen mixture PMFs; environment/emissive inputs remain excluded unless Stage 0 admitted them.
4. Route initial reservoir resolve into the same raw lobe/visibility boundary.
5. Delete the old reservoir representation and obsolete initialization code in the same change; no internal selector keeps both.

### Exit Gate

- `CHK-DIR-02` passes selection-frequency, normalization, zero-support, extreme-weight, and deterministic replay cases.
- Initial-reservoir output agrees statistically with exhaustive truth.
- History/reprojection is disabled or freshly invalid every frame; this stage makes no temporal claim.

### Ready-To-Use Prompt

> Implement Direct Lighting Stage 3 only. Replace the old direct reservoir and initial sampling in one clean break using the frozen target, source PMF/PDF, integer sample identity, stream update, bounded `M`, and normalization. Keep temporal/spatial reuse off. Support only the candidate families admitted by `DIR-D0`. Compare raw output statistically to the exhaustive baseline and run finite/invariant tests. Delete the superseded representation, callers, shaders, bindings, and registrations; do not add a compatibility mode.

## Stage 4 — Add Correct Temporal And Spatial Reuse

### Objective

Add stable-light temporal reuse, disocclusion handling, spatial reuse, and the selected bias-correction modes without stale samples.

### Work

1. Add current/previous light-generation views and stable index translation at the existing GPU-scene preparation boundary.
2. Implement frozen reprojection and compatibility fields, classified invalidation, unique random dimensions, temporal `M` handling, and selected correction.
3. Add decorrelated spatial neighbors and disocclusion-specific policy; expose only durable quality profiles.
4. Publish confidence/reuse classification required by reconstruction, without adding a diagnostic subsystem.
5. Run mutation, long-frame-identity, dual-view, resize, reload, and provider-switch sequences.

### Exit Gate

- `AC-DIR-04/05/09` pass raw reservoir/lobe checks.
- `CHK-DIR-03/05` show no wrong-light reuse, non-finite state, cross-view contamination, or unexplained ghost energy.
- Active biased/corrected mode is reported honestly and matches the frozen equations.

### Ready-To-Use Prompt

> Implement Direct Lighting Stage 4 on the accepted Stage-3 reservoir. Add previous/current light translation, complete surface/history identity, temporal and spatial reuse, disocclusion policy, and the frozen bias-correction modes. Keep reservoir and reconstruction history separate. Exercise every `CHK-DIR-03/05` mutation, including light reorder/removal, material/alpha change, occluder motion, cuts, resize, dual views, shader reload, and long frame indices. Stop if any correction equation or history field remains unresolved.

## Stage 5 — Prove Visibility Semantics And Provider Parity

### Objective

Make selected-sample visibility reliable across Inline/Pipeline providers and D3D12/Vulkan capability cells.

### Work

1. Centralize the frozen semantic segment, robust offset, alpha, two-sided, culling, light-distance, and failure rules in one reusable ray semantic kernel.
2. Keep Inline and Pipeline execution/binding owners distinct; remove semantic drift and obsolete adapters.
3. Validate strict/automatic requested-versus-active status and injected missing program/SBT/TLAS/material/capability faults.
4. Test grazing/contact/thin/alpha/animated geometry and finite area-light samples.
5. Run architecture-boundary validation because Renderer/RHI ray boundaries may be affected.

### Exit Gate

- `AC-DIR-06` and `CHK-DIR-04` pass at predeclared tolerances for supported cells.
- Unavailable cells fail explicitly without unshadowed substitution.
- No native handle/device-address/backend-specific identity leaks into the feature frontend.

### Ready-To-Use Prompt

> Implement Direct Lighting Stage 5. Reconcile Inline and Pipeline selected-light visibility to the frozen segment, offset, face, alpha, and material semantics while retaining their real execution differences. Exercise strict/automatic resolution and inject missing capability/program/SBT/TLAS/material faults. Run the required Renderer/RHI architecture boundary check and paired-provider fixtures. Do not add screen traces, VSM, opacity micromaps, or a generic shadow-provider framework.

## Stage 6 — Deliver Portable Reconstruction

### Objective

Turn the proved raw sparse signal into a stable product image without hiding estimator errors or making an optional vendor path mandatory.

### Work

1. Implement/adapt the accepted vendor-neutral direct reconstruction baseline through the existing reconstruction owner.
2. Provide exact raw radiance, hit distance, motion, depth, normals, roughness/lobe class, confidence/disocclusion, exposure, and history identity.
3. Integrate optional DLSS RR only through the same semantic boundary and explicit readiness/fallback policy.
4. Validate static convergence, subpixel motion, cuts, disocclusion, animated lights/occluders/materials, alpha foliage, thin geometry, and high-energy light toggles.
5. Measure raw and reconstructed memory/time independently.

### Exit Gate

- `AC-DIR-07`, `FM-DIR-06`, and `CHK-DIR-06` pass.
- Raw outputs remain capturable and any bias/lag/light loss is visible in evidence.
- Mandatory hardware retains a supported baseline; optional RR failure has the frozen response.

### Ready-To-Use Prompt

> Implement Direct Lighting Stage 6 after raw estimator and visibility gates pass. Add the accepted portable reconstruction path using the frozen guide/confidence/history contract, then connect optional DLSS Ray Reconstruction through the existing provider boundary. Preserve raw lobe capture and separate reservoir from denoiser history. Run the motion/disocclusion/light-toggle matrix and report raw versus reconstructed quality, memory, and time. Stop if the denoiser is required to make an invalid estimator look correct.

## Stage 7 — Admit And Deliver Many-Light Expansions

### Objective

Add only those candidate classes or spatial guiding structures whose measured value justifies their content, state, and update cost.

### Work

For each separately reviewed increment:

1. close its admission experiment and update `DIR-D0`, architecture, hook ledger, budgets, and checks;
2. add environment mapping/PDF, emissive-triangle inventory/update, or ReGIR world-space structures through existing scene/GPU-scene owners;
3. prevent double counting between emissive surface hits, environment background, and direct candidates;
4. stress dynamic updates, zero-power entries, distributed versus overlapping lights, and generation retirement;
5. delete the superseded simpler path only if the accepted product no longer needs it; retain uniform/exhaustive oracle modes where bounded.

### Exit Gate

- Each new class passes `AC-DIR-08/09` and its added failure matrix.
- Quality/time/memory win is demonstrated on the workload that admitted it.
- No duplicate light/material/asset pipeline or generic spatial-sampling framework appears.

### Ready-To-Use Prompt

> Implement only the Direct Lighting Stage-7 increment explicitly admitted by `DIR-D0` (environment candidates, emissive triangles, or ReGIR). Start from its measured baseline and frozen update/identity/PDF/budget contract. Integrate through existing scene, GPU-scene, material, asset, and frame-graph owners; prevent emission double counting; add mutation and retirement checks; and report the quality/time/memory delta. Do not bundle the other increments or advertise unlimited lights.

## Stage 8 — Product Adoption, Backend Evidence, And Closure

### Objective

Prove the complete candidate in representative content and publish an honest `FCR-REN-06` verdict.

### Work

1. Freeze candidate revision, build/configuration, shaders, backend/device/driver, content/reference hashes, settings, warmup/capture windows, and thresholds.
2. Run Cornell Box/analytic controls, Sponza Tier 0, Bistro interior/exterior, San Miguel held-out, synthetic overlap, motion/disocclusion, and controlled failure cells.
3. Run D3D12/Vulkan native validation for supported profiles and record unsupported cells explicitly.
4. Capture raw lobes, selected sample/visibility/confidence where required, reconstructed result, final preview, memory, and GPU timing distributions.
5. Complete adoption/settings/default/error-message checks, stale-path/build/package validation, scoped diff review, and `git diff --check`.
6. Publish only evidence actually observed to `FCR-REN-06`; leave open or blocked cells explicit.

### Exit Gate

- `AC-DIR-01` through `AC-DIR-12` have owned results.
- Required workload/backend/failure cells pass or the feature remains blocked with exact reasons.
- No superseded source path, stale documentation link, hidden fallback, or unsubmitted required artifact remains.

### Ready-To-Use Prompt

> Execute Direct Lighting Stage 8 as candidate validation and adoption. Freeze exact candidate/configuration/reference identity and thresholds before runs. Exercise analytic/Cornell, Sponza, Bistro, San Miguel, overlap, motion, disocclusion, alpha, failure, and D3D12/Vulkan native-validation cells. Retain raw and reconstructed artifacts and separate numerical, temporal, visual, performance, memory, backend, and release verdicts. Update `FCR-REN-06` only with observed evidence; do not repair unrelated defects during the evidence run.

## Stage-To-Acceptance Traceability

| Stage | Acceptance | Failures/checks |
| --- | --- | --- |
| 0 | scope for all | `DIR-X-*`; `FM-DIR-D0-*`; discovery package |
| 1 | `AC-DIR-01/02/03` foundation | `FM-DIR-01`; `CHK-DIR-01` |
| 2 | `AC-DIR-01/02/03` production semantics | `CHK-DIR-01/09` |
| 3 | `AC-DIR-03/04` initial estimator | `FM-DIR-04`; `CHK-DIR-02` |
| 4 | `AC-DIR-04/05/09` | `FM-DIR-02/03/04/07`; `CHK-DIR-03/05/07` |
| 5 | `AC-DIR-06/10` partial | `FM-DIR-03/05/08`; `CHK-DIR-04/08` |
| 6 | `AC-DIR-07/09` | `FM-DIR-06/07`; `CHK-DIR-06/07` |
| 7 | `AC-DIR-08/09/11` | increment-specific plus `CHK-DIR-07/09` |
| 8 | `AC-DIR-10/11/12` and closure | `CHK-DIR-08/09`; `FCR-REN-06` |

## Stop And Escalation Rules

Stop the active stage when:

- its semantic, source pin, tolerance, or failure response is not frozen;
- a cheaper oracle disagrees with the proposed optimization;
- evidence depends on tone-mapped presentation or a shared unproved implementation;
- the change needs a new public type, feature-named external hook, asset format, RHI capability, third-party dependency, or release expansion not admitted by the stage;
- current/previous resources cannot be kept generation-safe within the memory budget;
- a backend/provider produces different semantic results and the cause is unknown;
- unrelated/concurrent work overlaps the owned paths and cannot be reconciled safely.

Record the exact blocker and preserve the last valid candidate. Do not widen the stage, hide the cell, or mark it passed.

## Completion Rule

The plan is complete only when Stage 8 closes the required cells in `FCR-REN-06`. Research, accepted architecture, compiled code, a responsive process, a stable screenshot, or performance on one GPU is insufficient alone.
