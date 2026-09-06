# Offline Path Tracer Discovery Acceptance

Status: binding acceptance contract for `PTD-00` discovery; not feature implementation acceptance

Responsibility: define the evidence required to decide what a complete offline unbiased path tracer means for SparkleEngine and whether an implementation plan may be created

Authority boundary: the [research report](../../Research/OfflinePathTracerCompletion.md) owns precedent and initial findings, the [roadmap](../../Strategy/Roadmap.md#offline-reference-truth-first) owns priority, [`FCR-REN-08`](../FeatureCompletionReports.md#initial-completion-report-registry) owns eventual feature closure, and a future plan may own delivery only after this gate passes

Current state: **In progress / blocked for exit** on 2026-09-06. Initial source research exists; accepted transport scope, estimator derivation, executable evidence design, target architecture decision, and independent review do not.

## Iteration Control Record

| Field | `ITER-PTD-00` |
| --- | --- |
| Claim | Sparkle has enough reviewed, falsifiable information to plan the smallest offline path tracer that can serve as a bounded correctness oracle. |
| North Stars | `NS-REAL`, `NS-MATH-DATA`, `NS-EVIDENCE`, `NS-OWNERSHIP`, `NS-SIMPLIFY` |
| Persona targets | `PGE-02`, `PGE-05`, `PGE-06`, `PGE-07`, `PGE-08`, `PGE-09`, `PGE-10`, `PGE-13`, `PGE-15` |
| Delivery targets | `PTD-00`; preparation for `FCR-REN-08`, `REL-04`, `REL-05`, `MAP-A` through `MAP-H` |
| Primary release risks | `RISK-REL-05`, `RISK-REL-06`, `RISK-REL-08`, `RISK-REL-11`, `RISK-REL-12`, `RISK-REL-13` |
| Decision | `BLOCKED`; only discovery evidence work is authorized. `PTD-01` implementation-plan creation requires every criterion below to pass. |

## Discovery Scope

Included:

- a formal transport-domain and unbiasedness claim;
- current Sparkle owner/producer/consumer/lifetime/build/backend/dependency trace;
- primary NVIDIA precedent and rejected-copy decisions;
- feature, dependency-independence, risk, failure, oracle, fixture, statistical, artifact, and operational definitions;
- a target-shape decision with alternatives and complexity budget;
- plan-ready work packages, owners, dependencies, clean breaks, evidence order, and estimates.

Excluded:

- production implementation, permanent test code, renderer architecture changes, shader changes, external renderer integration, and candidate/reference image generation;
- declaring the current `ReferencePathTraced` route unbiased, converged, ground truth, feature complete, or shippable;
- changing eventual feature acceptance merely because discovery is complete.

## Discovery Exit Criteria

Every row must be `PASS`. “Documented,” “reviewed,” and “accepted” mean the exact retained artifact is linked and a named reviewer can reproduce the decision from it. Unknowns may remain only when they are converted into an owned implementation-plan check with a bounded consequence that cannot change target architecture, feature scope, estimator correctness, or release claims.

| ID | Pass criterion | Required retained evidence | Blocks exit when |
| --- | --- | --- | --- |
| `AC-PTD-01` | Terms and claimant are frozen: offline, reference, unbiased, consistent, converged, feature complete, ground truth, raw, preview, and candidate comparison each have a non-overlapping meaning. | terminology/claim table approved by feature and evidence owners | A result can change labels without changing evidence. |
| `AC-PTD-02` | The target transport equation, measures, radiometric units, path domain, camera model, supported scene snapshot, and full-versus-finite path-length claim are explicit. | reviewed mathematical specification and notation | “Unbiased” has no exact integral or a deterministic cutoff is hidden. |
| `AC-PTD-03` | Every camera, geometry/deformation, material/texture, alpha/sidedness, light, environment, emission, and dynamic-scene behavior is included or excluded for `v0.1`. | feature-domain matrix reconciled with release maps and public selectors | An advertised or map-required semantic has no verdict. |
| `AC-PTD-04` | Current implementation and build membership are traced from selection through scene/view data, primary hit, sampling, lighting, traversal, accumulation, output, and failure on D3D12/Vulkan. | line-linked current-route trace with owner, producer, consumer, lifetime, format, configuration, fallback, and invalidation | Any contribution, clamp/cutoff, state transition, or backend route is untraced. |
| `AC-PTD-05` | The complete estimator is derived: strategy/lobe/light selection, BSDF/emission/environment terms, PDFs/Jacobians, MIS if used, delta events, NEE, Russian roulette, termination, and zero/invalid cases. | independently reviewed derivation plus hand-computable examples | A term or probability appears in code without its estimator role, or vice versa. |
| `AC-PTD-06` | Oracle independence is sufficient for every future comparison claim. Shared dependencies are listed and each has another defect-detecting oracle. | dependency matrix and oracle ladder with fault examples | The reference and subject can preserve the same bug without detection. |
| `AC-PTD-07` | Sampling identity and determinism are defined independently of frame timing and work scheduling. | seed/sample/dimension allocation, stream identity/version, repeat/resume rules, and correlation-test design | Samples can repeat, skip, or change silently across interruption/backend/scheduling. |
| `AC-PTD-08` | Numeric robustness is specified for ray spawn/endpoints, geometric versus shading normals, transforms/scale, PDFs/weights, finite values, overflow/underflow, accumulation precision, and counters. | numeric policy, adversarial fixture definitions, thresholds, and invalid-result behavior | A scene-tuned epsilon, silent clamp, NaN/Inf, cap hit, or counter loss can enter accepted radiance. |
| `AC-PTD-09` | Accumulation and job lifecycle are deterministic and transactional in the target: static snapshot, reset/invalidation, exact sample count, precision, checkpoint/resume, cancellation, partial output, overflow, and atomic completion are decided. | state machine and artifact lifecycle decision | Stale or partial history can be mistaken for a complete render. |
| `AC-PTD-10` | The oracle fixture ladder covers analytic, metamorphic, minimal-independent, external-interchange, backend-parity, lifecycle, and representative-map cases. | fixture catalog with source rights, scene/camera/light/material equivalence, expected result, and named defect per case | A broad beauty scene is the first or only correctness check. |
| `AC-PTD-11` | Statistical acceptance distinguishes bias, variance, correlation, convergence, and finite-sample outliers. | predeclared replicate counts, sample sequence, regions, confidence/uncertainty, stop rules, thresholds, and escalation | One seed/SPP/image/average score can pass the oracle. |
| `AC-PTD-12` | Raw evidence cannot be confused with presentation. | linear-HDR/AOV/diagnostic/provenance schema and separate preview/display transform contract | Denoising, clamp/filter, exposure, tone map, gamut, encoding, or screenshot path contaminates raw comparison. |
| `AC-PTD-13` | Backend/capability/failure behavior and operational bounds are finite. | D3D12/Vulkan matrix; validation/capability/device-loss behavior; maximum duration/VRAM/disk; progress, timeout, cancel, cleanup, and recovery contract | Unsupported hardware silently falls back, failure looks complete, or a job has no bounded stop. |
| `AC-PTD-14` | Every material risk and failure mode has prevention, detection, contingency, owner, retirement evidence, and at least one check capable of exposing it. | completed risk/failure/check ledger with dry-run review | A check only confirms execution or a risk has no observable retirement evidence. |
| `AC-PTD-15` | The selected target shape is the smallest one that satisfies the frozen claim; alternatives and exclusions are evidence-backed. | decision record comparing GBuffer-seeded/camera-ray, minimal/full, megakernel/wavefront, sampling, accumulation, export, and backend options | Selection depends only on precedent, familiarity, elegance, or speculative performance. |
| `AC-PTD-16` | The implementation-planning handoff is complete without beginning implementation. | ordered plan-input backlog with owners, dependencies, clean-break deletions, estimates/ranges, key check per slice, and unchanged/excluded surfaces | Planning would need to reopen scope, architecture, estimator, evidence, or ownership decisions. |
| `AC-PTD-17` | A reviewer outside the author path can reconstruct the claim and identify at least one way each evidence class could fail. | signed review record with questions, corrections, unresolved items, and final decision | The report requires private explanation or review finds a non-detecting check. |

## Discovery Failure Modes

These are discovery no-go conditions. They describe what the discovery process must expose and control; eventual runtime failure modes are finalized in the feature acceptance contract after discovery.

| ID | Failure | Required discovery response |
| --- | --- | --- |
| `FM-PTD-01` | “Reference,” “offline,” “unbiased,” or “ground truth” is used as a product name without a bounded mathematical/evidence meaning. | Fail `AC-PTD-01`/`02`; remove the claim or freeze its exact domain. |
| `FM-PTD-02` | The subject and oracle share primary visibility, GBuffer, material decode, BRDF, light, traversal, accumulation, or display behavior that can mask the target defect. | Fail `AC-PTD-06`; add an independent oracle for the shared leaf or narrow the comparison claim. |
| `FM-PTD-03` | Deterministic bounce/distance cutoff, contribution clamp, firefly filter, denoiser, cache, biased stop, or sample rejection changes expectation without being labeled. | Fail `AC-PTD-02`/`05`/`12`; remove it from raw output or declare the bounded biased target. |
| `FM-PTD-04` | A selection probability, PDF measure conversion, MIS weight, delta event, NEE contribution, emission/environment hit, or roulette compensation is missing, duplicated, or mismatched. | Fail `AC-PTD-05`; require a derivation and a check that distinguishes the faulty term. |
| `FM-PTD-05` | Frame timing, scheduling, backend order, interruption, or resume changes sample identity; dimensions alias or correlate. | Fail `AC-PTD-07`; define stable sample/dimension identity and correlation/repeat checks. |
| `FM-PTD-06` | Camera, scene, material, light, shader, setting, resolution, or backend changes without invalidating accumulation, or a partial job appears complete. | Fail `AC-PTD-09`/`12`; bind output to full identity and require atomic completion. |
| `FM-PTD-07` | Fixed ray epsilon, shading-normal offset, large coordinate, non-uniform scale/shear, thin gap, grazing ray, or endpoint error creates acne or light leak. | Fail `AC-PTD-08`; adopt a justified robust policy and adversarial scale/transform tests. |
| `FM-PTD-08` | A release material/light/alpha/geometry behavior is missing, approximated differently, or present only in appearance rather than transport. | Fail `AC-PTD-03`/`10`; implement later, exclude from release, or narrow the oracle claim. |
| `FM-PTD-09` | External comparison uses different camera, units, spectra/color, texture decode, normal convention, material model, light shape, environment, or path domain. | Mark result `Inconclusive`; repair the interchange manifest before using the difference. |
| `FM-PTD-10` | Noise decreases while the mean converges to the wrong value, correlated sequences create false stability, or one aggregate metric hides local failures. | Fail `AC-PTD-10`/`11`; require independent replicates, analytic cases, regions/crops, uncertainty, and fault-sensitive metrics. |
| `FM-PTD-11` | D3D12 and Vulkan differ beyond declared numeric tolerance, strict capability is unavailable, or native validation reports an unexplained issue. | Fail `AC-PTD-13`; hold the affected backend/claim and preserve raw diagnostics. |
| `FM-PTD-12` | Exposure, tone mapping, gamut conversion, output encoding, filtering, denoising, or screenshot quantization is included in raw error measurement. | Fail `AC-PTD-12`; compare linear HDR before presentation and label previews separately. |
| `FM-PTD-13` | The job hangs, TDRs, exhausts memory/disk, cannot report progress, cannot cancel, or leaves ambiguous partial artifacts. | Fail `AC-PTD-09`/`13`; define budgets, watchdog, atomic output, cleanup, and recovery. |
| `FM-PTD-14` | NVIDIA feature breadth or an abstract framework expands the target beyond the frozen Sparkle release need. | Fail `AC-PTD-15`; reduce to the smallest accepted domain and defer the rest. |
| `FM-PTD-15` | A proposed check proves only that code ran, uses the implementation as its own expected value, or sets thresholds after seeing output. | Fail `AC-PTD-10`/`11`/`14`; predeclare the oracle and inject or identify the defect it detects. |
| `FM-PTD-16` | Discovery “passes” with an unresolved item capable of changing architecture, scope, estimator math, release claims, or evidence design. | Keep `PTD-00` blocked; unresolved architecture-shaping unknowns may not be transferred as ordinary implementation tasks. |

## Check Design Ledger

Each check specification produced during discovery must include initial state, action/injection, oracle, matrix, artifacts, maximum duration/resources, cleanup, and escalation as required by [Validation and Evidence](../../Engineering/Verification/ValidationAndEvidence.md#check-and-test-design-contract). This ledger authorizes design and review, not permanent submitted test code.

| Check | Falsifies | Minimum action and oracle | Required artifact | Escalation |
| --- | --- | --- | --- | --- |
| `CHK-PTD-01` current-route reconciliation | `AC-PTD-03`/`04`; `FM-PTD-02`/`08` | Search public selectors, build membership, owners, CPU graph, shader includes, resources, history, output, and both backend bindings; every contribution and fallback must land in one matrix row. | revision-pinned line links, dependency graph, unmatched-search record | Any unmatched selector/contribution blocks scope. |
| `CHK-PTD-02` primary-precedent audit | `AC-PTD-01`/`10`/`15`; `FM-PTD-01`/`14` | Pin NVIDIA revisions; inspect actual Falcor minimal/full tracer, accumulation/error, tests, RTXPT reference/real-time switches, and robustness source; distinguish adopted, rejected, and unknown lessons. | source URL/SHA ledger and comparison table | A secondary summary or unpinned moving claim cannot decide target shape. |
| `CHK-PTD-03` estimator review | `AC-PTD-02`/`05`; `FM-PTD-03`/`04` | Expand every sampled path probability and contribution in one notation; hand-evaluate zero, unit, delta, two-strategy, roulette, cutoff, and invalid cases. | derivation, reviewer annotations, hand-case outputs | Any unmatched term/probability blocks planning. |
| `CHK-PTD-04` dependency fault analysis | `AC-PTD-06`; `FM-PTD-02` | For each shared component, posit a concrete wrong value/branch and show which independent level detects it. | shared-dependency/fault/oracle matrix | A fault surviving all proposed oracles forces new independence or narrower claims. |
| `CHK-PTD-05` fixture and interchange review | `AC-PTD-03`/`10`; `FM-PTD-08`/`09` | Specify analytic/metamorphic/minimal/external/lifecycle/backend cases with exact scene semantics and expected failures; dry-run one case on paper. | fixture catalog, rights/provenance, interchange manifest, dry-run review | Any non-equivalent external scene is `Inconclusive`. |
| `CHK-PTD-06` sample-stream protocol | `AC-PTD-07`/`11`; `FM-PTD-05`/`10` | Define repeat, interruption/resume, cross-scheduling, distribution, correlation, replicate, and sequence-prefix checks with predeclared statistics. | sample identity spec and analysis recipe | Frame-index-only or order-dependent identity blocks target acceptance. |
| `CHK-PTD-07` accumulation-state protocol | `AC-PTD-08`/`09`; `FM-PTD-06`/`13` | Compare reference sums/averages at increasing counts and after every mutation/resume/overflow/cancel state; define precision and atomicity oracle. | state diagram, numeric tolerances, mutation matrix, partial-artifact rules | A stale/partial result that can appear complete blocks planning. |
| `CHK-PTD-08` robustness matrix | `AC-PTD-08`/`10`; `FM-PTD-07` | Define scale, translation, shear, mirrored transform, grazing, thin-gap, adjacent/coplanar triangle, shading-normal, and connection-endpoint cases against analytic geometry. | scene definitions, expected visibility, thresholds, backend matrix | A manually tuned per-scene epsilon is not an oracle. |
| `CHK-PTD-09` raw-output/provenance review | `AC-PTD-09`/`12`; `FM-PTD-06`/`12` | Trace raw radiance to file and comparison before every display operation; mutate each identity field and require mismatch/invalidation. | artifact schema, sample manifest, hash/invalidation table | Missing provenance or mixed raw/preview path blocks evidence use. |
| `CHK-PTD-10` backend/failure protocol | `AC-PTD-13`; `FM-PTD-11`/`13` | Specify strict D3D12/Vulkan capability checks, native validation, compiler/settings identity, device removal/TDR/OOM/disk-full/timeout/cancel behavior, cleanup, and numeric parity. | matrix and expected diagnostic/artifact states | Silent fallback or unexplained validation output blocks the backend. |
| `CHK-PTD-11` traceability audit | `AC-PTD-14`/`16`; `FM-PTD-15`/`16` | Mechanically and manually verify every AC/risk/FM has an owner and detecting check, every work package has one key check, and no plan-shaping unknown is deferred. | coverage table and unmatched-ID output | Any orphan ID or non-detecting check blocks exit. |
| `CHK-PTD-12` independent plan-readiness review | `AC-PTD-15`/`16`/`17`; `FM-PTD-14`/`16` | A non-author reconstructs the target, rejects alternatives from evidence, challenges one failure per evidence class, and decides whether planning requires reopening discovery. | signed review record and `PASS`/`BLOCKED` decision | Private explanation or reopened core decision keeps `PTD-00` blocked. |

## Required `PTD-00` Evidence Package

The discovery completion report links, rather than duplicates, these artifacts:

1. frozen terminology, claimant, transport equation/domain, units, and feature/exclusion matrix;
2. revision-pinned local route and NVIDIA source ledgers;
3. end-to-end estimator derivation and hand cases;
4. owner/producer/consumer/lifetime/build/backend and shared-dependency graph;
5. target-shape decision with rejected alternatives and complexity budget;
6. risk register with trigger, prevention, detection, contingency, owner, and retirement evidence;
7. `AC-*`/`FM-*`/`CHK-*` coverage table with no orphan;
8. fixture/oracle/interchange/statistical protocols and predeclared thresholds;
9. sample identity, accumulation/job state, raw-output/provenance, and controlled-failure contracts;
10. ordered, estimated, owner-assigned plan-input backlog plus independent review decision.

Generated experiment output belongs under the normal ignored evidence/artifact root selected by the evidence owner, not in `Docs`. Documentation retains manifests, summaries, decisions, and links; it does not become an image/blob archive.

## Gate Decision

`PTD-00` passes only when `AC-PTD-01` through `AC-PTD-17` pass, every applicable `FM-PTD-*` has a detecting `CHK-PTD-*`, all release/technical risks have accepted treatments, and independent review signs the exact report revision.

Current decision: **BLOCKED**. The next permitted step is `PTD-D0` through `PTD-D4` discovery evidence work. After a pass, the release owner may authorize `PTD-01` to create the implementation plan. That plan's existence will not authorize implementation before the roadmap opens the `FCR-REN-08` slice.
