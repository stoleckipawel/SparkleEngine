# Reference Path Tracer — Discovery Gate

**Status:** acceptance contract; binding feature-local discovery gate for `PTD-00`, not feature implementation acceptance

**Responsibility:** define the evidence required to decide what a complete, explicitly bounded and potentially unbiased Reference Path Tracer means for SparkleEngine, freeze the conditional implementation plan, and authorize production implementation

**Authority boundary:** the [research report](Research.md) owns precedent and initial findings, [Transport And Estimator](TransportAndEstimator.md) owns the proposed mathematical contract and decision slots, [Execution Architecture](ExecutionArchitecture.md) owns the proposed system boundary, [User Experience](UserExperience.md) owns the proposed human/automation workflow, the [conditional plan](Plan.md) owns Stage 0 and the provisional delivery route, the [roadmap](../../../../../../../Strategy/Roadmap.md#reference-path-tracer-truth-first) owns priority, the [feature dossier](README.md) owns the eventual feature set and definition of done, and [`FCR-REN-08`](../../../../../../../Acceptance/FeatureCompletionReports.md#initial-completion-report-registry) owns its result

**Current state:** **Stage 0 executed / BLOCKED** in report `PTD-00-R0` on 2026-09-10 at committed `master` revision `669637cf23b9748f8b94635409e74159d31d0bc2`. The estimator, target architecture, evidence protocol, and UX choices have a candidate freeze; exit is prohibited by the unfrozen `ReleaseMapSet`/support matrix, unresolved public/package reachability, and independent reviews that correctly reproduced those blockers.

**Naming reconciliation:** the 2026-09-09 working-tree clean break makes `ReferencePathTracer` the sole feature name; it changes no behavior, authorization, or evidence claim.

**Current readiness:** **20/100** for the feature baseline — this discovery contract is in progress and adds no implementation or verification credit until its evidence is accepted. See [Current Feature Readiness](../../../../../../../Acceptance/CurrentReadiness.md#renderer).

## Gate At A Glance

| Discovery must freeze | Why planning cannot safely infer it later |
| --- | --- |
| exact transport target, measures, units, supported scene/camera/material/light domain, and meaning of “unbiased” | these choices determine the estimator and whether the eventual oracle claim is truthful |
| complete current route and shared-dependency inventory | a reference path that shares the subject's defect can agree while both are wrong |
| estimator derivation, sampling identity, numeric policy, and per-view accumulation/invalidation state machine | implementation order and data ownership depend on them; post-hoc fixes can invalidate all produced references |
| viewport-first product pillar: view-mode placement, automatic start, responsive live navigation, newest-camera presentation, automatic post-motion refinement, truthful progress, Editor/Game camera reset, Lit comparison retention, and secondary export | these semantics and their priority decide the View/Renderer/RHI/Application boundary and cannot be invented after the estimator or artifact route is embedded in frame history |
| analytic through representative oracle ladder with thresholds, budgets, artifacts, and reviewers | a beauty scene cannot localize bias, lifecycle, determinism, robustness, or backend defects |
| chosen target shape, rejected alternatives, owners, clean breaks, and plan-ready work packages | otherwise the plan would be architecture discovery disguised as delivery |

`PASS` authorizes freezing `PTD-01` to the exact discovery report and permits Stage 1 only after `REL-03`; it does not accept the feature, approve a release claim, or make the current route trustworthy. Any unresolved question that can change the target integral, supported domain, architecture, ownership, or evidence validity keeps this gate blocked.

## Iteration Control Record

| Field | `ITER-PTD-00` |
| --- | --- |
| Claim | Sparkle has enough reviewed, falsifiable information to plan the smallest Reference Path Tracer that can serve as a bounded correctness oracle. |
| North Stars | `NS-REAL`, `NS-MATH-DATA`, `NS-EVIDENCE`, `NS-OWNERSHIP`, `NS-SIMPLIFY` |
| Persona targets | `PGE-02`, `PGE-05`, `PGE-06`, `PGE-07`, `PGE-08`, `PGE-09`, `PGE-10`, `PGE-13`, `PGE-15` |
| Delivery targets | `PTD-00`; preparation for `FCR-REN-08`, `REL-04`, `REL-05`, `MAP-A` through `MAP-H` |
| Primary release risks | `RISK-REL-05`, `RISK-REL-06`, `RISK-REL-08`, `RISK-REL-11`, `RISK-REL-12`, `RISK-REL-13` |
| Primary technical risks | `RISK-PTD-01` through `RISK-PTD-12` |
| Decision | `BLOCKED`; only discovery evidence work and Stage 0 of the conditional `PTD-01` plan are authorized. Production implementation requires every criterion below plus `REL-03` to pass. |

## Discovery Scope

Included:

- a formal transport-domain and unbiasedness claim;
- current Sparkle owner/producer/consumer/lifetime/build/backend/dependency trace;
- primary NVIDIA, AMD, and neutral precedent plus adopted/rejected-copy decisions;
- feature, dependency-independence, risk, failure, oracle, fixture, statistical, artifact, and operational definitions;
- a target-shape decision with alternatives and complexity budget;
- plan-ready work packages, owners, dependencies, clean breaks, evidence order, and estimates.

Excluded:

- production implementation, permanent test code, renderer architecture changes, shader changes, external renderer integration, and candidate/reference image generation;
- declaring the current `ReferencePathTracer` route unbiased, converged, ground truth, feature complete, or shippable;
- changing eventual feature acceptance merely because discovery is complete.

## Discovery Exit Criteria

Every row must be `PASS`. “Documented,” “reviewed,” and “accepted” mean the exact retained artifact is linked and a named reviewer can reproduce the decision from it. Unknowns may remain only when they are converted into an owned implementation-plan check with a bounded consequence that cannot change target architecture, feature scope, estimator correctness, or release claims.

| ID | Pass criterion | Required retained evidence | Blocks exit when |
| --- | --- | --- | --- |
| `AC-PTD-01` | Terms and claimant are frozen: Reference Path Tracer, unbiased, consistent, converged, feature complete, ground truth, raw, preview, and candidate comparison each have a non-overlapping meaning. | terminology/claim table approved by feature and evidence owners | A result can change labels without changing evidence. |
| `AC-PTD-02` | The target transport equation, measures, radiometric units, path domain, camera model, supported scene snapshot, and full-versus-finite path-length claim are explicit. | accepted revision of [Transport And Estimator](TransportAndEstimator.md), completed decision slots, hand cases, and independent review | “Unbiased” has no exact integral, a symbol/choice remains implicit, or a deterministic cutoff is hidden. |
| `AC-PTD-03` | Every camera, geometry/deformation, material/texture, alpha/sidedness, light, environment, emission, and dynamic-scene behavior is included or excluded for `v0.1`. | feature-domain matrix reconciled with release maps and public selectors | An advertised or map-required semantic has no verdict. |
| `AC-PTD-04` | Current implementation and build membership are traced from selection through scene/view data, primary hit, sampling, lighting, traversal, accumulation, output, and failure on D3D12/Vulkan. | line-linked current-route trace with owner, producer, consumer, lifetime, format, configuration, fallback, and invalidation | Any contribution, clamp/cutoff, state transition, or backend route is untraced. |
| `AC-PTD-05` | The complete estimator is derived: strategy/lobe/light selection, BSDF/emission/environment terms, PDFs/Jacobians, MIS if used, delta events, NEE, Russian roulette, termination, and zero/invalid cases. | completed `MATH-*` equation-to-code ledger, independently reviewed derivation, and hand-computable examples | A term or probability appears in code without its estimator role, or vice versa. |
| `AC-PTD-06` | Oracle independence is sufficient for every future comparison claim. Shared dependencies are listed and each has another defect-detecting oracle. | dependency matrix and oracle ladder with fault examples | The reference and subject can preserve the same bug without detection. |
| `AC-PTD-07` | Sampling identity and determinism are defined independently of frame timing and work scheduling. | seed/sample/dimension allocation, stream identity/version, repeat/resume rules, and correlation-test design | Samples can repeat, skip, or change silently across interruption/backend/scheduling. |
| `AC-PTD-08` | Numeric robustness is specified for ray spawn/endpoints, geometric versus shading normals, transforms/scale, PDFs/weights, finite values, overflow/underflow, accumulation precision, and counters. | numeric policy, adversarial fixture definitions, thresholds, and invalid-result behavior | A scene-tuned epsilon, silent clamp, NaN/Inf, cap hit, or counter loss can enter accepted radiance. |
| `AC-PTD-09` | Per-view accumulation and lifecycle are deterministic and transactional: automatic validation/start, exact committed/target count, complete Editor/Game camera and scene invalidation, presentation/scheduling non-invalidation, Lit suspension/revalidation, precision, restart/checkpoint/resume, partial output, overflow, and atomic completion are decided. | classified invalidation matrix, view-session state machine, camera-producer matrix, and artifact lifecycle decision | Stale/mixed history can survive a change, a valid prefix is needlessly lost, or partial/target-SPP output can be mistaken for converged/complete evidence. |
| `AC-PTD-10` | The oracle fixture ladder covers analytic, metamorphic, minimal-independent, external-interchange, backend-parity, lifecycle, and representative-map cases. | fixture catalog with source rights, scene/camera/light/material equivalence, expected result, and named defect per case | A broad beauty scene is the first or only correctness check. |
| `AC-PTD-11` | Statistical acceptance distinguishes bias, variance, correlation, convergence, and finite-sample outliers. | predeclared replicate counts, sample sequence, regions, confidence/uncertainty, stop rules, thresholds, and escalation | One seed/SPP/image/average score can pass the oracle. |
| `AC-PTD-12` | Raw evidence cannot be confused with presentation. | linear-HDR/AOV/diagnostic/provenance schema and separate preview/display transform contract | Denoising, clamp/filter, exposure, tone map, gamut, encoding, or screenshot path contaminates raw comparison. |
| `AC-PTD-13` | The viewport-first product and backend/capability/failure behavior are finite, discoverable, and actionable: Reference Path Tracer is immediately after Lit, starts automatically, remains responsive during Editor/Game navigation, presents the newest camera identity within budget, refines automatically when motion stops, exposes exact progress/reset/completion, supports controlled Lit comparison, and later retains bounded raw-save/offscreen evidence routes. | D3D12/Vulkan matrix; accepted [User Experience](UserExperience.md) revision and P0-P3 priority; menu/state/camera/invalidation/response-budget/export design; validation/device-loss behavior; maximum duration/VRAM/disk; timeout, cancel, cleanup, recovery, accessibility, support, and Shipping reachability contract | The mode is hidden behind Lighting/CVars/a wizard, silently falls back, freezes or displays stale composition while navigating, requires manual post-motion restart, mixes changed views, loses valid comparisons, lets artifacts delay the viewport milestone, failure looks complete, or a session has no bounded stop. |
| `AC-PTD-14` | Every material risk and failure mode has prevention, detection, contingency, owner, retirement evidence, and at least one check capable of exposing it. | completed risk/failure/check ledger with dry-run review | A check only confirms execution or a risk has no observable retirement evidence. |
| `AC-PTD-15` | The selected target shape is the smallest one that satisfies the frozen claim; alternatives and exclusions are evidence-backed. | decision record comparing GBuffer-seeded/camera-ray, minimal/full, megakernel/wavefront, sampling, accumulation, export, and backend options | Selection depends only on precedent, familiarity, elegance, or speculative performance. |
| `AC-PTD-16` | The implementation-planning handoff is complete without beginning implementation. | ordered plan-input backlog with owners, dependencies, clean-break deletions, estimates/ranges, key check per slice, and unchanged/excluded surfaces | Planning would need to reopen scope, architecture, estimator, evidence, or ownership decisions. |
| `AC-PTD-17` | A reviewer outside the author path can reconstruct the claim and identify at least one way each evidence class could fail. | signed review record with questions, corrections, unresolved items, and final decision | The report requires private explanation or review finds a non-detecting check. |

## Discovery Risk Register

Risk status is recorded in `PTD-00-R0` below. Discovery activity alone does not reduce a risk; only the named retirement evidence does. The owner named here is accountable for obtaining a real assignee in the completion report.

| ID | Risk and observable trigger | Prevention | Detection | Contingency and owner | Retirement evidence |
| --- | --- | --- | --- | --- | --- |
| `RISK-PTD-01` | False oracle through shared dependencies: a deliberately wrong GBuffer/material/light/BRDF value survives both subject and reference. | Prohibit production GBuffer and post-process inputs; inventory all allowed shared leaves. | `CHK-PTD-01`, `CHK-PTD-04`, analytic/minimal/external fault cases. | Narrow the oracle claim or add independent coverage; Renderer/evidence owners. | Every shared leaf has a demonstrated defect-detecting oracle and no unexplained survivor. |
| `RISK-PTD-02` | Estimator bias or double counting: derivation/code has an unmatched term, probability, measure, cutoff, or rejected sample. | Freeze one transport equation/notation and review strategy composition before architecture selection. | `CHK-PTD-03` plus analytic probability/energy cases. | Remove the term/optimization or relabel the finite/biased target; path-integrator owner. | Independent derivation review and all hand/analytic cases pass. |
| `RISK-PTD-03` | Feature-domain mismatch: a release map/selector exercises an unmodeled or differently approximated material, light, camera, geometry, or alpha behavior. | Reconcile domain against `ReleaseMapSet` and all public selectors during `PTD-D0`. | `CHK-PTD-01`, `CHK-PTD-05`, per-feature fixture ledger. | Exclude the feature/map or expand discovery before plan freeze/Stage 1; release and graphics-quality owners. | Zero unmatched advertised/map-required semantic. |
| `RISK-PTD-04` | Numeric ray/transport failure: scale, transform, grazing, thin geometry, shading normal, extreme PDF, or radiance produces acne, leak, NaN/Inf, overflow, or lost energy. | Specify robust spawn/endpoints, normal policy, finite checks, precision, and invalid-result rules. | `CHK-PTD-03`, `CHK-PTD-07`, `CHK-PTD-08`. | Block the affected domain/backend and retain diagnostic output; Renderer/RHI owners. | Adversarial numeric matrix passes with zero unaccounted invalid value or visibility error. |
| `RISK-PTD-05` | Sample or accumulation identity drifts: repeated/skipped/correlated samples, unobserved Editor/Game camera or scene change, needless presentation reset, stale Lit-return history, wrong resume count, or partial completion. | Stable session/pixel/sample/dimension identity plus classified per-view invalidation and transactional accumulation. | `CHK-PTD-06`, `CHK-PTD-07`, `CHK-PTD-09`. | Discard the affected prefix, preserve only verified checkpoint/artifacts, repair the missing identity producer, and invalidate dependents; View/Renderer sampling owners. | Repeat/resume/camera/scene/presentation/mode-switch/overflow matrix and independent sequence review pass. |
| `RISK-PTD-06` | Cross-renderer agreement/disagreement is misread because scenes are not semantically equivalent. | Freeze an interchange manifest for camera, units, geometry, material, texture, light, environment, and path domain. | `CHK-PTD-05` plus analytic controls rendered by both routes. | Mark comparison `Inconclusive`; never tune thresholds to force agreement; content/evidence owners. | Equivalence checklist and analytic controls pass before external images influence verdicts. |
| `RISK-PTD-07` | False convergence: correlation or one aggregate metric hides a stable wrong mean or local failure. | Predeclare independent replicates, regions, statistics, uncertainty, and stop rules. | `CHK-PTD-05`, `CHK-PTD-06`; analytic bias injections and failure crops. | Increase independent evidence or hold the claim `Inconclusive`; evidence owner. | The protocol detects injected bias/correlation/local error and all required regions pass. |
| `RISK-PTD-08` | Backend/compiler/capability divergence changes output or failure behavior. | One semantic contract, strict requested/active reporting, pinned compiler/settings, and no silent fallback. | `CHK-PTD-01`, `CHK-PTD-10`, native validation and paired numeric comparison. | Exclude the failing backend or repair before oracle use; Renderer/RHI owners. | Both advertised backends pass capability, validation, controlled failure, and numeric tolerance. |
| `RISK-PTD-09` | Raw truth is contaminated by clamp/filter/denoiser/exposure/tone/encode/screenshot operations. | Give raw linear HDR a separate named resource/export and immutable metadata; label previews. | `CHK-PTD-03`, `CHK-PTD-09` with each biasing switch toggled or rejected. | Reject contaminated artifacts and regenerate from raw; evidence owner. | Raw path trace proves no display/biasing operation before comparison and identity mutation is detected. |
| `RISK-PTD-10` | Reference accumulation hangs, TDRs, exhausts VRAM/disk, cannot cancel, or leaves a plausible partial result. | Freeze time/resource budgets, progress, watchdog, cancellation, atomic publish, cleanup, and recovery. | `CHK-PTD-07`, `CHK-PTD-10` controlled timeout/OOM/disk/cancel cases. | Terminate safely, preserve diagnostic/partial state as non-candidate, and reduce bounded workload; runtime owner. | Every controlled failure ends within budget with no complete marker, leaked state, or corrupted prior evidence. |
| `RISK-PTD-11` | External feature breadth grows a second scene/material framework or multiple competing reference paths. | Select the smallest target from current release needs and enforce one owner/representation/copy budget. | `CHK-PTD-02`, `CHK-PTD-12`; dependency and code-shape estimates. | Defer nonessential features and reject generalized infrastructure; Renderer/release owners. | Accepted decision records each excluded external feature and plan has one target path with bounded size. |
| `RISK-PTD-12` | Production implementation begins, or a conditional plan choice is treated as frozen, while discovery can still change architecture, scope, estimator, or evidence. | Hard `PTD-00` Stage-1 gate; label the plan conditional and make Stage 0 its only authorized work. | `CHK-PTD-11`, `CHK-PTD-12`; inspect code/worktree changes, plan status, prerequisites, and unresolved questions. | Stop premature implementation, revert provisional authority to target status, and return to the unresolved discovery slice; release owner. | All `AC-PTD-*` pass, zero plan-shaping unknowns remain, independent review approves the exact plan revision, and Stage 1 records that revision. |

## Discovery Failure Modes

These are discovery no-go conditions. They describe what the discovery process must expose and control; eventual runtime failure modes are finalized in the feature acceptance contract after discovery.

| ID | Failure | Required discovery response |
| --- | --- | --- |
| `FM-PTD-01` | “Reference Path Tracer,” “unbiased,” or “ground truth” is used as a product name without a bounded mathematical/evidence meaning. | Fail `AC-PTD-01`/`02`; remove the claim or freeze its exact domain. |
| `FM-PTD-02` | The subject and oracle share primary visibility, GBuffer, material decode, BRDF, light, traversal, accumulation, or display behavior that can mask the target defect. | Fail `AC-PTD-06`; add an independent oracle for the shared leaf or narrow the comparison claim. |
| `FM-PTD-03` | Deterministic bounce/distance cutoff, contribution clamp, firefly filter, denoiser, cache, biased stop, or sample rejection changes expectation without being labeled. | Fail `AC-PTD-02`/`05`/`12`; remove it from raw output or declare the bounded biased target. |
| `FM-PTD-04` | A selection probability, PDF measure conversion, MIS weight, delta event, NEE contribution, emission/environment hit, or roulette compensation is missing, duplicated, or mismatched. | Fail `AC-PTD-05`; require a derivation and a check that distinguishes the faulty term. |
| `FM-PTD-05` | Frame timing, scheduling, backend order, interruption, or resume changes sample identity; dimensions alias or correlate. | Fail `AC-PTD-07`; define stable sample/dimension identity and correlation/repeat checks. |
| `FM-PTD-06` | Effective Editor/Game camera, scene, material, light, shader, transport setting, resolution, or backend changes without invalidating before mixing; presentation/mode scheduling resets needlessly; Lit return resumes stale work; or a partial prefix appears converged/complete. | Fail `AC-PTD-09`/`12`/`13`; classify identity versus presentation/scheduling, bind output to full identity, expose the reset reason, and require exact/atomic completion. |
| `FM-PTD-07` | Fixed ray epsilon, shading-normal offset, large coordinate, non-uniform scale/shear, thin gap, grazing ray, or endpoint error creates acne or light leak. | Fail `AC-PTD-08`; adopt a justified robust policy and adversarial scale/transform tests. |
| `FM-PTD-08` | A release material/light/alpha/geometry behavior is missing, approximated differently, or present only in appearance rather than transport. | Fail `AC-PTD-03`/`10`; implement later, exclude from release, or narrow the oracle claim. |
| `FM-PTD-09` | External comparison uses different camera, units, spectra/color, texture decode, normal convention, material model, light shape, environment, or path domain. | Mark result `Inconclusive`; repair the interchange manifest before using the difference. |
| `FM-PTD-10` | Noise decreases while the mean converges to the wrong value, correlated sequences create false stability, or one aggregate metric hides local failures. | Fail `AC-PTD-10`/`11`; require independent replicates, analytic cases, regions/crops, uncertainty, and fault-sensitive metrics. |
| `FM-PTD-11` | D3D12 and Vulkan differ beyond declared numeric tolerance, strict capability is unavailable, or native validation reports an unexplained issue. | Fail `AC-PTD-13`; hold the affected backend/claim and preserve raw diagnostics. |
| `FM-PTD-12` | Exposure, tone mapping, gamut conversion, output encoding, filtering, denoising, or screenshot quantization is included in raw error measurement. | Fail `AC-PTD-12`; compare linear HDR before presentation and label previews separately. |
| `FM-PTD-13` | The job hangs, TDRs, exhausts memory/disk, cannot report progress, cannot cancel, or leaves ambiguous partial artifacts. | Fail `AC-PTD-09`/`13`; define budgets, watchdog, atomic output, cleanup, and recovery. |
| `FM-PTD-14` | NVIDIA/AMD feature breadth or an abstract framework expands the target beyond the frozen Sparkle release need. | Fail `AC-PTD-15`; reduce to the smallest accepted domain and defer the rest. |
| `FM-PTD-15` | A proposed check proves only that code ran, uses the implementation as its own expected value, or sets thresholds after seeing output. | Fail `AC-PTD-10`/`11`/`14`; predeclare the oracle and inject or identify the defect it detects. |
| `FM-PTD-16` | Discovery “passes” with an unresolved item capable of changing architecture, scope, estimator math, release claims, or evidence design. | Keep `PTD-00` blocked; unresolved architecture-shaping unknowns may not be transferred as ordinary implementation tasks. |

## Check Design Ledger

Each check specification produced during discovery must include initial state, action/injection, oracle, matrix, artifacts, maximum duration/resources, cleanup, and escalation as required by [Validation and Evidence](../../../../../../../Engineering/Verification/ValidationAndEvidence.md#check-and-test-design-contract). This ledger authorizes design and review, not permanent submitted test code.

| Check | Criteria, failures, and risks falsified | Minimum action and oracle | Required artifact | Escalation |
| --- | --- | --- | --- | --- |
| `CHK-PTD-01` current-route reconciliation | `AC-PTD-03`, `AC-PTD-04`; `FM-PTD-02`, `FM-PTD-08`; `RISK-PTD-01`, `RISK-PTD-03`, `RISK-PTD-08` | Search public selectors, build membership, owners, CPU graph, shader includes, resources, history, output, and both backend bindings; every contribution and fallback must land in one matrix row. | revision-pinned line links, dependency graph, unmatched-search record | Any unmatched selector/contribution blocks scope. |
| `CHK-PTD-02` primary-precedent audit | `AC-PTD-01`, `AC-PTD-10`, `AC-PTD-15`; `FM-PTD-01`, `FM-PTD-14`; `RISK-PTD-11` | Pin NVIDIA/AMD revisions; inspect Falcor minimal/full tracer, RTXPT reference controls, NVIDIA robustness, Capsaicin shared Inline/DXR tracer, Baikal's bias/wavefront/workflow disclosures, RadeonRays separation, and neutral math/format sources; distinguish adopted, rejected, and unknown lessons. | source URL/SHA ledger and comparison table | A secondary summary or unpinned moving claim cannot decide target shape. |
| `CHK-PTD-03` estimator review | `AC-PTD-02`, `AC-PTD-05`; `FM-PTD-03`, `FM-PTD-04`; `RISK-PTD-02`, `RISK-PTD-04`, `RISK-PTD-09` | Expand every sampled path probability and contribution in one notation; hand-evaluate zero, unit, delta, two-strategy, roulette, cutoff, and invalid cases. | derivation, reviewer annotations, hand-case outputs | Any unmatched term/probability blocks planning. |
| `CHK-PTD-04` dependency fault analysis | `AC-PTD-06`; `FM-PTD-02`; `RISK-PTD-01` | For each shared component, posit a concrete wrong value/branch and show which independent level detects it. | shared-dependency/fault/oracle matrix | A fault surviving all proposed oracles forces new independence or narrower claims. |
| `CHK-PTD-05` fixture and interchange review | `AC-PTD-03`, `AC-PTD-10`; `FM-PTD-08`, `FM-PTD-09`; `RISK-PTD-03`, `RISK-PTD-06`, `RISK-PTD-07` | Specify analytic/metamorphic/minimal/external/lifecycle/backend cases with exact scene semantics and expected failures; dry-run one case on paper. | fixture catalog, rights/provenance, interchange manifest, dry-run review | Any non-equivalent external scene is `Inconclusive`. |
| `CHK-PTD-06` sample-stream protocol | `AC-PTD-07`, `AC-PTD-11`; `FM-PTD-05`, `FM-PTD-10`; `RISK-PTD-05`, `RISK-PTD-07` | Define repeat, interruption/resume, cross-scheduling, distribution, correlation, replicate, and sequence-prefix checks with predeclared statistics. | sample identity spec and analysis recipe | Frame-index-only or order-dependent identity blocks target acceptance. |
| `CHK-PTD-07` accumulation-state protocol | `AC-PTD-08`, `AC-PTD-09`, `AC-PTD-13`; `FM-PTD-06`, `FM-PTD-13`; `RISK-PTD-04`, `RISK-PTD-05`, `RISK-PTD-10` | Compare reference sums/averages and viewport state after every Editor/Game camera field, rapid/continuous movement, motion stop, scene mutation, target-SPP update, presentation/scheduling change, Lit switch/return, restart/resume/overflow/cancel state; define exact reset reason, prefix, precision, retention, newest-view response budget, automatic post-motion refinement, and later artifact-atomicity oracles. | state diagram, camera-producer matrix, invalidation classification, response/numeric tolerances, partial-artifact rules | Any stale/mixed prefix, frozen or old-camera presentation beyond budget, manual post-motion start, unexplained reset, or partial/target-SPP result that can appear converged/complete blocks planning. |
| `CHK-PTD-08` robustness matrix | `AC-PTD-08`, `AC-PTD-10`; `FM-PTD-07`; `RISK-PTD-04` | Define scale, translation, shear, mirrored transform, grazing, thin-gap, adjacent/coplanar triangle, shading-normal, and connection-endpoint cases against analytic geometry. | scene definitions, expected visibility, thresholds, backend matrix | A manually tuned per-scene epsilon is not an oracle. |
| `CHK-PTD-09` raw-output/provenance review | `AC-PTD-09`, `AC-PTD-12`; `FM-PTD-06`, `FM-PTD-12`; `RISK-PTD-05`, `RISK-PTD-09` | Trace raw radiance to file and comparison before every display operation; mutate each identity field and require mismatch/invalidation. | artifact schema, sample manifest, hash/invalidation table | Missing provenance or mixed raw/preview path blocks evidence use. |
| `CHK-PTD-10` backend/failure protocol | `AC-PTD-13`; `FM-PTD-11`, `FM-PTD-13`; `RISK-PTD-08`, `RISK-PTD-10` | Specify strict D3D12/Vulkan capability checks, native validation, compiler/settings identity, device removal/TDR/OOM/disk-full/timeout/cancel behavior, cleanup, and numeric parity. | matrix and expected diagnostic/artifact states | Silent fallback or unexplained validation output blocks the backend. |
| `CHK-PTD-11` traceability audit | `AC-PTD-14`, `AC-PTD-16`; `FM-PTD-15`, `FM-PTD-16`; `RISK-PTD-12` | Mechanically and manually verify every AC/risk/FM has an owner and detecting check, every work package has one key check, and no plan-shaping unknown is deferred. | coverage table and unmatched-ID output | Any orphan ID or non-detecting check blocks exit. |
| `CHK-PTD-12` independent plan-readiness review | `AC-PTD-15`, `AC-PTD-16`, `AC-PTD-17`; `FM-PTD-14`, `FM-PTD-16`; `RISK-PTD-11`, `RISK-PTD-12` | A non-author reconstructs the target, rejects alternatives from evidence, challenges one failure per evidence class, and decides whether planning requires reopening discovery. | signed review record and `PASS`/`BLOCKED` decision | Private explanation or reopened core decision keeps `PTD-00` blocked. |

## Required `PTD-00` Evidence Package

The discovery completion report links, rather than duplicates, these artifacts:

1. frozen terminology, claimant, accepted [Transport And Estimator](TransportAndEstimator.md) revision, completed `MATH-*` ledger, transport equation/domain, units, and feature/exclusion matrix;
2. revision-pinned local route plus NVIDIA, AMD, and neutral source ledgers;
3. end-to-end estimator derivation and hand cases;
4. owner/producer/consumer/lifetime/build/backend and shared-dependency graph;
5. target-shape decision with rejected alternatives and complexity budget;
6. risk register with trigger, prevention, detection, contingency, owner, and retirement evidence;
7. `AC-*`/`FM-*`/`CHK-*` coverage table with no orphan;
8. fixture/oracle/interchange/statistical protocols and predeclared thresholds;
9. sample identity, per-view accumulation/invalidation/session state, raw-output/provenance, and controlled-failure contracts;
10. accepted [User Experience](UserExperience.md) revision covering its P0-P3 delivery order, Reference Path Tracer immediately after Lit, automatic preflight/start, mode-safe defaults, responsive live navigation, newest-camera presentation, automatic post-motion refinement, exact progress/reset/completion, Editor/Game camera invalidation, Lit comparison retention/revalidation, pause/restart, secondary checkpoint/raw save/offscreen equivalence, errors, accessibility, first use, and Shipping exclusion;
11. ordered, estimated, owner-assigned plan-input backlog plus independent math, numerical, architecture, evidence, and first-use review decisions.

Generated experiment output belongs under the normal ignored evidence/artifact root selected by the evidence owner, not in `Docs`. Documentation retains manifests, summaries, decisions, and links; it does not become an image/blob archive.

## `PTD-00-R0` Stage-0 Execution Report

### Identity, method, and verdict

| Field | Result |
| --- | --- |
| Report | `PTD-00-R0` |
| Repository input | committed `master` `669637cf23b9748f8b94635409e74159d31d0bc2`; clean worktree before Stage-0 documentation edits |
| Authority used | live code/build configuration, current acceptance/roadmap authorities, this dossier, and pinned primary-source ledger |
| Cheapest falsifiers | targeted `rg`/file inspection, selector/build/source trace, identifier reconciliation, and independent read-only reviews; no build or render was needed to prove the blocking release/scope facts |
| Production changes | none permitted and none made |
| Decision | **BLOCKED**. `AC-PTD-03`, `AC-PTD-13`, and `AC-PTD-17` cannot pass without a frozen `ReleaseMapSet` plus accepted hardware/package reachability and a subsequent independent review of that exact reconciliation. Stage 1 is not authorized; `REL-03` is independently blocked. |

This is a real Stage-0 result, not a claim that document completeness proves the feature. The mathematical and product choices below are frozen candidates so the missing release inputs cannot be disguised as implementation discretion. When the external blockers close, `PTD-00-R1` must rebase to the then-current commit, reconcile only the affected matrices, repeat all independent reviews, and either accept the exact dossier revision or record new blockers.

### `PTD-D0` terminology, claimant, and scope decision

| Term | Frozen meaning |
| --- | --- |
| Reference Path Tracer | The development-only viewport product and equivalent automation route defined by this dossier; a name alone grants no oracle authority. |
| `SurfaceTransportReference` | The unidirectional camera-path estimator of the full accepted opaque reflective surface integral in [Transport And Estimator](TransportAndEstimator.md), with compensated stochastic termination and no valid hard path/distance cap. |
| `FinitePathDiagnostic(D)` | The explicitly finite diagnostic in the same contract. `D>=1`; it cannot be relabeled or selected as the full product. |
| Unbiased | The expectation equals the named mathematical target for the frozen domain. It says nothing about completeness of physics, finite-sample error, or shared-dependency independence. |
| Consistent | Independent prefixes/replicates approach that target under the frozen sampler and finite-variance assumptions. |
| Converged | The predeclared regional statistical protocol passes; reaching target SPP is only prefix completion. |
| Raw | Scene-linear linear-sRGB/D65 radiance-equivalent data before display, denoising, reconstruction, clamp, exposure, gamut, encoding, or screenshot conversion. |
| Preview | A one-way display derivative that names its transform and raw digest and is never numeric oracle input. |
| Reference candidate | A completed, valid raw prefix plus manifest that has not yet earned claim-specific oracle authority. |
| Ground truth | Permitted only per comparison claim after the dependency matrix supplies an independent detector for the defect class and uncertainty passes. Never a universal label. |
| Claimant | The `FCR-REN-08` acceptance owner, not the implementer, UI, file name, or this discovery report. |

The target shape is accepted as one Renderer-owned per-view session, independent camera rays, one semantic megakernel-first estimator, thin Inline/RGS traversal adapters, binary64 transactional accumulation, live viewport presentation, and secondary evidence publication. A wavefront scheduler may be considered only after profiling and semantic equivalence; a GBuffer seed, separate batch renderer, duplicate scene/material system, multiple estimators, or compatibility selector is rejected.

### Frozen feature-domain and release reconciliation

| Surface | `v0.1` candidate disposition | Exact boundary |
| --- | --- | --- |
| Camera | Included: perspective pinhole. Excluded: orthographic, panoramic, aperture/DOF, shutter/motion blur, animated-time integration. | Non-perspective or non-frozen time rejects before sample zero. |
| Geometry | Included: indexed triangles, instances, static and immutable evaluated skin/morph snapshots, affine transforms including nonuniform scale/shear/mirror when robust checks pass. | Curves, points, procedural primitives/displacement, changing deformation inside a prefix are excluded. |
| Textures | Included: 2D base mip, frozen bilinear/address/color decode and UV transform. | Streaming/mip/anisotropic/derivative changes are excluded from raw v0.1. |
| Materials | Included: opaque or deterministic mask; base color, metallic, roughness, fixed dielectric F0, normal map, emissive, one-/two-sided reflective surface. | Blend/transmission, physical subsurface/BSSRDF, volume, spectral, clearcoat/sheen/iridescence/anisotropy/procedural lobes are excluded. AO is never transport input. |
| Lights | Included: point/spot, directional, rectangle, emissive triangles, and environment under the exact unit/PDF contract. | IES, portals, textured analytic lights, arbitrary light shaders, and undeclared falloff/range truncation are excluded. |
| Backend/frontend | Included target: D3D12 Inline, D3D12 RGS, Vulkan Inline, Vulkan RGS, each strict with requested/active truth and identical estimator/sample semantics. | A missing pair is an excluded product row, never fallback. Current source has only compute/Inline. |
| Raw output | Included target: exact-extent binary64 prefix state, FLOAT OpenEXR beauty/AOVs, counters/statistics, canonical manifest, SHA-256, optional checkpoint. | BMP/screenshot/UI-scaled/display output is preview only. |
| Workflow | Included target: DevelopmentEditor live `Scene` view and DevelopmentGame `Game` view semantics; exact offscreen request is secondary. | Debug console/CVar choreography is diagnostic only. |
| Package | DevelopmentEditor includes selector/UI/CLI/writer after acceptance. DevelopmentGame may expose the semantic only through an approved development UI. ShippingEditor and ShippingGame exclude selector, CVar, CLI, writer, operation, docs promise, and optional dependencies. | Current recursive source/link membership does not establish this exclusion; package proof is required. |

`RPT-FS-01` through `RPT-FS-16` are **Included candidate scope**, `RPT-FS-17` and `RPT-FS-18` are **Excluded**, and `RPT-FS-19` is **Prohibited in raw**. This disposition is not release-ratified: `ReleaseMapSet` does not exist yet. Any admitted map or public selector reaching an excluded row blocks `PTD-00-R1` or forces an explicit scope reopening before implementation.

### Included feature ownership and proof budget

| Rows | Owner / primary phase | Defect-detecting checks | Budget | Independent/shared-dependency oracle |
| --- | --- | --- | --- | --- |
| `RPT-FS-01`, `10`, `12`, `15` | Renderer View/session, Editor/Application adapters; Stages 1, 6, 7 | `CHK-RPT-02`, `07`, `09`, `15`, `16` | 255-455 h combined stages; per UX runtime bounds | canonical camera fixtures, state-model traces, non-author first use |
| `RPT-FS-02`, `03`, `11` | Renderer View/geometry plus RHI traversal; Stages 2, 5 | `CHK-RPT-03`, `04`, `08` | 130-220 h | CPU analytic rays/triangles, NVIDIA/PBRT error-bound challenge, paired backends |
| `RPT-FS-04`, `05`, `06` | Scene/material/texture producers and Renderer BSDF; Stages 2, 4, 5 | `CHK-RPT-03`, `04`, `05` | 220-370 h overlapping | glTF fixtures, CPU decode/eval, furnace, Mitsuba-equivalent scenes |
| `RPT-FS-07`, `08`, `09` | Renderer light/integrator; Stages 3, 4 | `CHK-RPT-05`, `11` | 160-270 h | analytic energy/PDF cases, minimal tracer, Falcor/Capsaicin/Mitsuba |
| `RPT-FS-13`, `14` | Renderer readback, ApplicationEditor publication, evidence owner; Stage 9 | `CHK-RPT-04`-`13` | 145-250 h | SHA-256 replay, analytic/minimal/external ladder, controlled faults |
| `RPT-FS-16` | release/evidence owner; Stage 10 and `PTD-03` | `CHK-RPT-14`, `16` | 70-125 h plus per-map execution | independent renderer and per-map acceptance owner |

The rows share stage work; budgets must not be summed by row. External precedent constrains design only and does not satisfy any Sparkle result.

### `PTD-D1` live route, build, and dependency audit

| Concern | Live source result at report input | Required clean break |
| --- | --- | --- |
| Selectors | [`RenderViewMode.h`](../../../../../../../../Engine/Renderer/Public/Debug/RenderViewMode.h#L5-L23) and [`ViewportTopPanel.cpp`](../../../../../../../../Engine/Editor/Private/Panels/ViewportTopPanel.cpp#L183-L229) contain Lit/debug only. [`LightingMode::ReferencePathTracer`](../../../../../../../../Engine/Renderer/Public/Settings/EngineRenderingRayTracingTypes.h#L18-L22) is global, appears in [`RenderingSettingsPanel.cpp`](../../../../../../../../Engine/Editor/Private/Panels/RenderingSettingsPanel.cpp#L68-L70), persists as `r.Lighting.Mode`, and is reachable through generic command-line/runtime CVar surfaces. | Add `RenderViewMode::ReferencePathTracer` immediately after Lit; remove the Lighting enum value, panel row, CVar/persistence/CLI meaning, and all producers/consumers in the same change. |
| Editor/Game view identity | The live enum is `Game`, `Scene`, `Preview`, `Thumbnail`, `Debug`; both [`UI.cpp`](../../../../../../../../Engine/Editor/Private/UI.cpp#L38-L44) and [`ViewportPanel.cpp`](../../../../../../../../Engine/Editor/Private/Panels/ViewportPanel.cpp#L33-L37) submit `Game`. | Use existing `Scene` for Editor-produced viewports and `Game` for runtime. Update both Editor producers; no invented `Editor` enum. |
| Render route | [`Lighting.cpp`](../../../../../../../../Engine/Renderer/Private/Passes/Lighting/Lighting.cpp#L24-L58) selects separate direct/indirect compute producers, common composite/sky, then frame-history accumulation. [`ReferencePathTracerIndirectLighting.hlsl`](../../../../../../../../Engine/Assets/Shaders/Passes/RayTracing/ReferencePathTracerIndirectLighting.hlsl#L28-L52) starts from `RayTracingGBufferPathSurface`; [`ReferencePathTracerDirectLighting.hlsl`](../../../../../../../../Engine/Assets/Shaders/Passes/RayTracing/ReferencePathTracerDirectLighting.hlsl#L69-L175) loops every analytic light and averages per-frame samples. | Replace with independent camera ray and one semantic estimator; delete the old producer topology after consumers migrate. Preserve real-time GBuffer/lighting only outside reference transport. |
| Sampling/termination | [`PathSampling.hlsli`](../../../../../../../../Engine/Assets/Shaders/RayTracing/PathSampling.hlsli#L25-L169) uses frame-derived randomness, selected-component diffuse/GGX and current roulette; [`PathTrace.hlsli`](../../../../../../../../Engine/Assets/Shaders/RayTracing/PathTrace.hlsli#L18-L44) uses fixed epsilon/bias/distance and back-face culling. | Replace completely with accepted `MATH-03`, `08`, `09`, and `11`; no compatibility stream/settings. |
| Scene/View ownership | [`PreparedRenderScene.h`](../../../../../../../../Engine/Renderer/Private/Scene/Preparation/PreparedRenderScene.h#L34-L54) owns canonical scene snapshots; [`RenderView.h`](../../../../../../../../Engine/Renderer/Private/View/RenderView.h#L18-L36) owns viewport/selection/kind/camera/extents/display; `RenderFrame` owns one of each. Current RPT state is one optional hash in [`RenderViewState.h`](../../../../../../../../Engine/Renderer/Private/View/RenderViewState.h#L97-L108) plus a history texture. | Lease immutable Scene generations and canonical View fingerprint into one Renderer per-view session. Do not copy a second scene or expose handles to ApplicationEditor. |
| Invalidation | [`ReferencePathTracerInvalidation.cpp`](../../../../../../../../Engine/Renderer/Private/Passes/Lighting/ReferencePathTracer/ReferencePathTracerInvalidation.cpp#L11-L23) hashes lighting state, SPP/bounce/bias/distance, global view mode, and matrices; [`ReferencePathTracerAccumulation.hlsl`](../../../../../../../../Engine/Assets/Shaders/Passes/RayTracing/ReferencePathTracerAccumulation.hlsl#L27-L55) additionally uses a motion-vector epsilon and float mean/count. | Replace with semantic digest, exact committed range, generation rejection, reason taxonomy, target-SPP continuation rules, and presentation/scheduling non-reset. |
| RHI frontends | The three [`ReferencePathTracer` registrations](../../../../../../../../Engine/Renderer/ShaderRegistrations/ReferencePathTracerIndirectLightingShaders.cpp#L5-L9) are compute shaders requiring Inline RayQuery. Generic D3D12/Vulkan Inline and RGS machinery exists; no RPT RGS adapter exists. | One estimator with thin four-pair traversal adapters; RHI owns mechanism/capability, Renderer owns path semantics. |
| Source/build/generated | [`Engine/Renderer/CMakeLists.txt`](../../../../../../../../Engine/Renderer/CMakeLists.txt#L8-L18) recursively includes private sources and lines 63-148 include registration sources in runtime/cook objects; Renderer links into ShowcaseEditor and ShowcaseRuntime. Existing cooked shader records are unbound generated artifacts, not report evidence. | Update sources, registrations, shader cook manifests/generated outputs, CMake/profile membership, and stale artifacts atomically. Audit all six configured profiles: `DebugEditor`, `DebugGame`, `DevelopmentEditor`, `DevelopmentGame`, `ShippingEditor`, and `ShippingGame`. |
| Capture/export | [`ViewportCaptureService`](../../../../../../../../Engine/Renderer/Private/Viewport/ViewportCaptureService.cpp#L75-L139) and both RHI backends provide asynchronous typed readback (capacity three), but [`EditorViewportCaptureCoordinator.cpp`](../../../../../../../../Engine/Application/Private/Editor/Capture/EditorViewportCaptureCoordinator.cpp#L14-L62) requests final SceneColor and writes BMP under workspace `Saved/Captures`. TinyEXR v1.0.7 is loader-owned by TextureCooker; no EXR writer route is present. | Extend typed RHI readback; add a development ApplicationEditor raw writer/transaction after dependency/license review. Never route raw reference through BMP or link TinyEXR into Shipping by convenience. |
| Application operations | [`EditorOperationService.cpp`](../../../../../../../../Engine/Application/Private/EditorOperations/EditorOperationService.cpp#L29-L148) has cancellable shader-recook and BMP-write work only; no reference operation/progress/offscreen manifest exists. | Add one bounded development operation consuming semantic Renderer snapshots; application owns paths/process results, not estimator state. |
| Package roots | Development artifacts and runtime discovery contracts exist, but no accepted staged package/installer root or Reference Path Tracer allowlist exists; capture/config still use workspace paths. | `REL-03` must freeze immutable install/mutable-data roots. Raw output uses an explicit writable destination and unique staging sibling. |
| Release maps | Acceptance and roadmap explicitly say `ReleaseMapSet` is not frozen and map rights/feature coverage are unresolved. | Content/release owners provide the exact manifest; reconcile every used camera/material/light/deformation/alpha semantic before `PTD-00-R1`. |

Shared leaves are allowed only with the following independent detector:

| Shared leaf / injected fault | Independent detector |
| --- | --- |
| Camera transform/projection (transpose, Y flip, half-pixel) | CPU known-ray oracle plus Mitsuba camera manifest; constant environment catches filter normalization. |
| Triangle decode/transform/barycentrics (index, winding, shear) | CPU analytic intersection and metamorphic rigid-transform/scale fixtures; paired frontend hit records. |
| Texture/material decode (sRGB-as-linear, wrong channel, tangent sign) | CPU texel/material evaluator, glTF known values, flat/normal-mapped furnace, external equivalent scene. |
| Light units/PDF (missing PMF/Jacobian, wrong cone/side) | hand energy/PDF cases and minimal event tracer; Mitsuba/Falcor only after equivalence. |
| Traversal/alpha (miss, cull, cutoff equality) | analytic occluders and forced Inline/RGS/D3D12/Vulkan hit/event parity. |
| Accumulation/output (lost ordinal, float count, display contamination) | host binary64 sequence, checkpoint round trip, raw hash, and injected display transform. |

### `PTD-Q-*` disposition

| Question | `PTD-00-R0` result |
| --- | --- |
| `PTD-Q-01` | Resolved: full `SurfaceTransportReference` plus distinct `FinitePathDiagnostic(D)`; no hard cap can produce the full label. |
| `PTD-Q-02` | Resolved: independent camera rays required; GBuffer seed deleted from the route. |
| `PTD-Q-03` | Resolved by accepted candidate `MATH-02` through `08` and retained hand cases. |
| `PTD-Q-04` | Resolved: one-light NEE plus power-heuristic MIS; emissive triangles and environment participate. |
| `PTD-Q-05` | **Blocked externally:** candidate domain is frozen above, but exact release-map/public-selector reconciliation awaits `ReleaseMapSet`. |
| `PTD-Q-06` | Resolved: transmission/media/physical subsurface are excluded and must be unreachable. |
| `PTD-Q-07` | Resolved by exact normal, side, alpha, and immutable deformation contracts. |
| `PTD-Q-08` | Resolved as a design by `MATH-11`; executable backend conservatism remains Stage-5/8 evidence, not implementation discretion. |
| `PTD-Q-09` | Resolved by Philox stream, fixed dimension ledger, digest, and clean-break checkpoint policy. |
| `PTD-Q-10` | Resolved by binary64 mean/M2, uint64 count, ordered range merge, and max `1,048,576` SPP. |
| `PTD-Q-11` | Resolved by exact-extent OpenEXR/checkpoint/canonical manifest/SHA-256 contract. |
| `PTD-Q-12` | Resolved: mean/M2/standard error, ray/path/path-length/event and first-invalid counters; no general dashboard. |
| `PTD-Q-13` | Resolved contractually: four strict pairs with no fallback; executable parity remains later feature evidence. |
| `PTD-Q-14` | Resolved by the predeclared protocol below. |
| `PTD-Q-15` | Resolved by the frozen [User Experience](UserExperience.md#frozen-defaults-and-operational-budgets) state/action/budget contract. |
| `PTD-Q-16` | Resolved fact: no current runtime/image/performance/package evidence exists. That absence blocks feature acceptance, not Stage-0 design; no evidence was fabricated. |

### `PTD-D2` oracle, fixtures, statistics, and fault protocol

Every check starts from a clean session and immutable manifest, performs one named action/injection, retains raw/event/counter output, and restores/removes local temporary data. Default maximums are `2 min` source/trace checks, `5 min` CPU algebra/sampler checks, `15 min` per analytic GPU pair, `30 min` per minimal scene/pair, `4 h` per external renderer scene/replicate, `10 min` per controlled operational fault, and `15 min` first-use. A timeout is `Inconclusive` or the named failure result, never `PASS`.

| Fixture/class | Exact oracle and injected defect |
| --- | --- |
| `ANA-CAMERA` | center/corner/subpixel rays, constant-radiance filter, projection rejection; inject transpose, Y flip, missing half-pixel. |
| `ANA-BSDF` | zero/unit Lambert, reciprocity where applicable, GGX normalization/furnace sweep, mirror limit; omit lobe PMF or use selected-component PDF. |
| `ANA-LIGHT` | point/directional delta, rectangle/triangle Jacobian, cone, sided emitter, uniform and one-hot environment; omit PMF, distance squared, `sin(theta)`, or emitter side. |
| `ANA-MIS-RR` | equal/zero PDF, emission-hit split, delta bypass, roulette expectation/tail; double emission, compare delta density, omit survival division, return black at safety depth. |
| `ANA-GEOM` | barycentric/winding/alpha-equality/two-side plus scale `10^-6..10^6`, translation, shear, mirror, grazing, adjacent/coplanar and thin gaps; replace robust bound with fixed epsilon. |
| `META` | rigid transform, uniform unit-preserving scale with corresponding light/camera transform, emitter permutation, strategy relabel, batch/reorder/pause/resume/backend invariance; inject frame-index seed or branch-shifted dimensions. |
| `MIN-EVENT` | repository-local temporary CPU scalar event walker using independent double math and no Renderer BRDF/light helper; compare bounded event/factor records, then remove probe. |
| `EXT-FALCOR`, `EXT-CAPSAICIN`, `EXT-MITSUBA` | author tiny original scenes from numeric manifests; do not redistribute vendor assets. Pin renderer commit/version/config/output hashes and prove camera, units, base-level textures, material, normals, light shape/side, environment, path domain, and color equivalence first. Non-equivalence is `Inconclusive`. |
| `FAULT-OPS` | unsupported capability, capacity, timeout, cancel, device loss, OOM/allocation refusal, read-only/full disk, corrupt checkpoint, hash mismatch, late completion, and shutdown; assert state, deadline, cleanup, preserved prefix/prior artifact, and no completion manifest. |

Use eight independently keyed replicates. Evaluate the predeclared SPP ladder `4096, 16384, 65536, 262144, 1048576`; never add a post-hoc seed or crop. Regions are whole image, fixed `8x8` tiles, and manifest-defined masks for each material, light footprint, emissive, environment, alpha edge, silhouette/grazing edge, and robustness gap. Analytic scalar/factor GPU results require absolute error `<=1e-6` or relative error `<=5e-5` versus binary64, with exact event/category/count equality. For each predeclared region compute one value per replicate as the unweighted arithmetic mean of per-pixel linear luminance `Y=0.2126R+0.7152G+0.0722B` over exactly the manifest-listed pixel centers, then compute the replicate mean and unbiased sample standard deviation; the two-sided interval is Student `t` with 7 degrees of freedom and per-look/per-hypothesis `alpha=0.01/(5M)` (Bonferroni), where five is the fixed SPP-ladder look count and `M` is the fixed number of region-plus-whole-image scalar hypotheses in the manifest. The interval must contain the analytic/independent target and the mean must satisfy absolute error `<=5e-4` plus relative error `<=0.5%` when target magnitude is `>=0.1`; darker targets use the absolute rule. External equivalent scenes additionally use luminance `NRMSE=sqrt(mean((Y-Yref)^2))/max(sqrt(mean(Yref^2)),0.1)`, requiring whole-image `<=1%` and each fixed region `<=2%`. Injected `1%` regional luminance bias and duplicate/aliased streams must fail. Pass at the first ladder point meeting every rule; fail when the corrected confidence interval excludes the target beyond threshold; otherwise escalate and report `Inconclusive` at maximum SPP.

Interchange manifests contain source rights, generator/tool revision, metres, camera matrix/FOV/crop/filter, triangle winding/transforms, texture bytes/transfer/address/filter, tangent convention, material formulas, light units/shape/side/PMF, environment mapping, product/depth/RR/MIS, color primaries/white, seed/replicate/SPP, backend/compiler, and hashes. Only project-authored analytic scene data may be retained; external source/assets remain recipes unless rights review explicitly permits redistribution.

A repository-nonmutating PowerShell binary64 probe rechecked the corrected hand cases at R0: midpoint quadrature of `(pi/2)sin(pi v)` over `v in [0,1]` returned `1.0000000000411178`; its divided solid-angle density and `1/(4pi)` both returned `0.079577471545947673`; roulette target `0.2` produced threshold `3355443`, exact mass `0.19999998807907104`, and compensation scale `1`; the area PDF returned `1`, BSDF-hit MIS returned `0.2`, and Welford samples `(1,3)` returned mean/M2/standard-error `(2,2,1)`. This is a bounded arithmetic sanity check, not independent implementation, compiler, or GPU evidence.

### `PTD-D3` architecture decision and clean-break ledger

| Selected | Rejected alternative and reason |
| --- | --- |
| One per-view Renderer session; Editor uses existing `Scene`, runtime uses `Game`. | Global lighting branch/frame history cannot represent independent per-view identity or Lit suspension. New `Editor` kind is needless vocabulary. |
| Camera-ray megakernel semantic core first, thin Inline/RGS adapters. | GBuffer seed shares primary defects. Wavefront-first adds scheduling/state before evidence requires it. Separate frontend estimators permit drift. |
| Fixed near-uniform integer-partition active-lobe/light PMFs plus NEE/MIS; uniform-sphere environment sampling in v0.1. | Exhaustive lights scales poorly; BSDF-only environment/emissive is impractical; texture/energy importance requires an exact finite-probability contract and may follow profiling as a separately reviewed optimization. |
| Binary64 mean/M2 and exact uint64 count. | RGBA32F mean/count stalls/loses variance at high SPP; EMA/Kahan-only state does not supply the required uncertainty contract. Strict float64 capability is preferable to silent weak precision. |
| Existing typed RHI readback plus development ApplicationEditor transaction. | Screenshot/BMP is LDR; Renderer file I/O violates ownership; a second batch renderer risks semantic drift. |

The path-level clean-break ledger at audited HEAD is:

| Disposition | Exact paths |
| --- | --- |
| Delete | `Engine/Assets/Shaders/Passes/RayTracing/ReferencePathTracerAccumulation.hlsl`; `ReferencePathTracerDirectLighting.hlsl`; `ReferencePathTracerIndirectLighting.hlsl`; `Engine/Assets/Shaders/RayTracing/ReferencePathTracerUniform.hlsli`; every file under `Engine/Renderer/Private/Passes/Lighting/ReferencePathTracer/`; `Engine/Renderer/Private/Passes/Lighting/Direct/ReferencePathTracerDirectLighting.{h,cpp}`; `Engine/Renderer/Private/Passes/Lighting/ReferencePathTracerIndirectLighting.{h,cpp}`; `Engine/Renderer/Private/Passes/RayTracing/ReferencePathTracer{Accumulation,DirectLighting,IndirectLighting}Shader.h`; `Engine/Renderer/Private/RayTracing/Effects/ReferencePathTracer/ReferencePathTracerAccumulationUniformData.h`, `ReferencePathTracerCVars.{h,cpp}`, `ReferencePathTracerSettings.{h,cpp}`, and `ReferencePathTracerUniformData.h`; `Engine/Renderer/ShaderRegistrations/ReferencePathTracer{Accumulation,DirectLighting,IndirectLighting}Shaders.cpp`. |
| Modify | `Engine/Renderer/Public/Settings/EngineRenderingRayTracingTypes.h`; `Engine/Renderer/Public/Settings/EngineRenderingSettings.h`; `Engine/Renderer/Public/Debug/RendererCVars.h`; `Engine/Renderer/Private/Debug/RendererCVars.cpp`; `Engine/Renderer/Private/Settings/EngineRenderingSettings.cpp`; `Engine/Renderer/Private/Settings/EngineRenderingSettingsRuntime.cpp`; `Engine/Editor/Private/Panels/RenderingSettingsPanel.cpp`; `Engine/Renderer/Private/Passes/Lighting/Lighting.cpp`; `Engine/Renderer/Private/Resources/History/FrameHistory.{h,cpp}`; `Engine/Renderer/Private/View/RenderViewState.{h,cpp}`; `Engine/Renderer/Private/Frame/FramePipeline.h`; `Engine/Renderer/Private/Frame/FramePipelineGraph.cpp`; `Engine/Renderer/CMakeLists.txt`; `Engine/Editor/Private/UI.cpp`; `Engine/Editor/Private/Panels/ViewportPanel.cpp`; `Config/DefaultEngine.ini` if its serialized numeric lighting mode changes after enum removal. |
| Regenerate, never hand-edit | `artifacts/dev/projects/Shared/cooked/Shaders/CookedShaderLibrary.slib`; `GlobalShaderMap.smap`; `ShaderDependencies.sdep`; `ShaderPackageRegistry.sreg`; `recook.signal`. Their current bytes are disposable local output and not PTD evidence. |
| Preserve/extend singular owners | Canonical Scene/View/GPU-scene/material/texture/traversal owners and typed RHI readback; exact new Stage-1/2 files are selected by the owning stage without moving those responsibilities. |

Before either stage edits, repeat `rg -l "ReferencePathTracer|Reference Path Tracer|ReferencePathTraced|LightingMode" Engine Config Projects CMakeLists.txt cmake` against that stage's HEAD and reconcile any path delta into this ledger. No alias, legacy stream, dual selector, or checkpoint migration reader is permitted. Stage 1 deletes the legacy selector/route/state files and edits their direct consumers; Stage 2 alone changes the two Editor `RenderViewKind::Game` producers to `Scene` with the camera/fingerprint slice.

### `PTD-D4` owners, revised estimates, and traceability

Accountable roles are Renderer feature owner (session/integrator/accumulation), Scene/content owners (immutable semantics), RHI owners (capability/traversal/readback), Editor/Application owner (view intent/UI/operation/publication), build/package owner (profile reachability), evidence owner (fixtures/statistics), content/provenance owner (`ReleaseMapSet`/rights), and release acceptance owner (final verdict). Named people are assigned when the release roster exists; lack of a person does not transfer authority to an implementer.

Revised ranges after the live audit are: Stage 0 `70-110 h` including external owner reconciliation/re-review; Stage 1 `50-85 h`; Stage 2 `80-130 h`; Stage 3 `75-125 h`; Stage 4 `110-180 h`; Stage 5 `90-150 h`; Stage 6 `80-135 h`; Stage 7 `70-120 h`; Stage 8 `90-150 h`; Stage 9 `145-250 h`; Stage 10 `70-125 h`; total `930-1,560 h`. The increase covers binary64 strict-capability work, four traversal pairs, exact Scene/Game migration, shader/package clean break, and independent external/statistical evidence. Release-map breadth may revise Stage 5/9/10 only through a new accepted report.

| Check | Owner | Max design/execution budget | Cleanup and escalation |
| --- | --- | --- | --- |
| `CHK-PTD-01` | architecture owner | 2 h source probe | no artifacts; unmatched selector/build route blocks |
| `CHK-PTD-02` | research/rights owners | 8 h plus legal review | links/notes only; moving/unlicensed input rejected |
| `CHK-PTD-03` | math/numeric reviewers | 16 h | temporary calculations removed; unmatched term blocks |
| `CHK-PTD-04` | evidence owner | 4 h | retain matrix; surviving shared fault narrows claim |
| `CHK-PTD-05` | content/evidence owners | 16 h design | retain manifests only; non-equivalence is inconclusive |
| `CHK-PTD-06` | sampling owner | 8 h design/probe | remove dumps; duplicate/correlation blocks |
| `CHK-PTD-07` | View/session owner | 8 h model dry-run | remove transient traces; stale/mixed prefix blocks |
| `CHK-PTD-08` | geometry/RHI owners | 8 h design/probe | remove probe scenes if test-only; tuned epsilon blocks |
| `CHK-PTD-09` | evidence/Application owners | 8 h schema review | remove staging; missing hash/identity blocks |
| `CHK-PTD-10` | RHI/runtime owners | 8 h protocol | cleanup tickets/staging; fallback/leak blocks |
| `CHK-PTD-11` | documentation owner | 2 h mechanical/manual | no artifacts; orphan ID blocks |
| `CHK-PTD-12` | independent reviewers | 8 h | retain signed result; reopened decision blocks |

Coverage is conjunctive: `CHK-PTD-01` covers `AC-03/04`; `02` covers `AC-01/10/15`; `03` covers `AC-02/05`; `04` covers `AC-06`; `05` covers `AC-03/10`; `06` covers `AC-07/11`; `07` covers `AC-08/09/13`; `08` covers `AC-08/10`; `09` covers `AC-09/12`; `10` covers `AC-13`; `11` covers `AC-14/16`; `12` covers `AC-15/16/17`. `FM-PTD-01..16` and `RISK-PTD-01..12` each appear in their detecting check rows above; mechanical `rg` must return every ID before R1 acceptance. `RISK-PTD-03`, `08`, `10`, and `12` remain Open because their release/support/review evidence is unavailable; all other risk treatments are accepted designs but retire only through later executable evidence.

### Acceptance and independent review record

| Criterion | R0 result | Evidence / blocker |
| --- | --- | --- |
| `AC-PTD-01` | PASS | frozen terminology/claimant table |
| `AC-PTD-02` | PASS (design) | frozen Transport revision and hand cases; not implementation proof |
| `AC-PTD-03` | **BLOCKED** | no accepted `ReleaseMapSet`; exact map-required semantics cannot be reconciled |
| `AC-PTD-04` | PASS (source) | revision-pinned live-route/build/dependency audit above |
| `AC-PTD-05` | PASS (design) | `MATH-01..11` dispositions, equations, hand cases, and equation-to-code owners |
| `AC-PTD-06` | PASS (design) | shared-leaf fault/oracle matrix |
| `AC-PTD-07` | PASS (design) | Philox packing/dimension/prefix rules |
| `AC-PTD-08` | PASS (design) | numeric/invalid/robust endpoint contract and adversarial matrix |
| `AC-PTD-09` | PASS (design) | state/invalidation/retention/artifact decisions plus UX budgets |
| `AC-PTD-10` | PASS (design) | analytic through external fixture catalog |
| `AC-PTD-11` | PASS (design) | replicate/SPP/region/confidence/threshold/stop protocol |
| `AC-PTD-12` | PASS (design) | raw/checkpoint/EXR/hash and preview separation |
| `AC-PTD-13` | **BLOCKED** | numeric budgets are frozen, but accepted hardware, maps, package roots, and Shipping reachability are not available to prove the contract is finite/reachable |
| `AC-PTD-14` | PASS (design) | AC/FM/RISK/CHK ownership and coverage record |
| `AC-PTD-15` | PASS (design) | selected/rejected target-shape record and revised complexity range |
| `AC-PTD-16` | **BLOCKED** | path-level ledger and stage ownership are now reconciled, but the exact report revision cannot be signed while release-owned prerequisites and independent reviews remain open |
| `AC-PTD-17` | **BLOCKED** | independent design re-reviews pass the candidate math, architecture, and UX contracts, but the exact immutable R1 dossier still lacks content/release, support/package, accountable human, and final cross-discipline signatures after release reconciliation |

Three independent read-only agent review tracks were conducted against R0 inputs: mathematical/numerical, architecture/current-route, and evidence/first-use. Initial passes returned **BLOCKED** and exposed the nonexistent `RenderViewKind::Editor`, incorrect environment Jacobian, discretely biased roulette, underspecified MIS/material/sampler/robustness/statistics/artifacts, incomplete lifecycle/CLI/Shipping behavior, current Inline/GBuffer/history route, LDR BMP capture, package reachability, and missing report/clean-break matrices. After reconciliation, the math/numeric and UX/evidence reviewers returned **PASS for the Stage-0 design candidate** with no plan-shaping invention point; the architecture reviewer found the architecture/plan internally coherent and **BLOCKED only by the declared external/evidentiary prerequisites**. These are AI-assisted internal reviews, not a substitute for named accountable human approval or executable evidence. R1 requires fresh independent math, numeric, architecture, evidence, content/release, support/package, and clean first-use signatures against one immutable candidate revision after the release-owned inputs exist.

### Unrun checks

No engine build, shader compile/cook, runtime launch, GPU validation, representative-map render, external-renderer run, package inspection, accessibility session, or clean-machine first use was run. They were unnecessary to falsify R0 exit and cannot be reported as passed. One repository-nonmutating PowerShell binary64 hand-case probe was run as recorded above; it is not implementation evidence. `git diff --check`, documentation-link/ID checks, and the production-file boundary audit are the other handoff validations appropriate after these documentation edits.

## Gate Decision

`PTD-00` passes only when `AC-PTD-01` through `AC-PTD-17` pass, every applicable `FM-PTD-*` has a detecting `CHK-PTD-*`, all release/technical risks have accepted treatments, and independent review signs the exact report revision.

Current decision: **`PTD-00-R0 BLOCKED` at `669637cf23b9748f8b94635409e74159d31d0bc2`**. Stage 0 has been executed; repeat only the affected release-map/support/package reconciliation and all independent reviews in `PTD-00-R1` after `REL-01` supplies the exact `ReleaseMapSet` and support identities. Stage 1 is **not authorized**. Neither this candidate freeze nor a future discovery pass authorizes implementation before `REL-03` opens the `FCR-REN-08` slice.
