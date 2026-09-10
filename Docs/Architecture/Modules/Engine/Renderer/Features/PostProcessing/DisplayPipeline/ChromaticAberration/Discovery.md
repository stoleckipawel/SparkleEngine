# Chromatic Aberration Discovery

**Status:** acceptance contract; `CHRD-00` is `Blocked`, and production implementation is not authorized

**Responsibility:** freeze the first-release lens model, units, color/order boundary, state, edge behavior, budgets, workflow, and evidence protocol before `DSP-6`

**Authority boundary:** [Research](Research.md) provides precedent; [Semantics](Semantics.md) and [Execution Architecture](ExecutionArchitecture.md) are design candidates; [Plan](Plan.md) is conditional delivery order; [README](README.md) owns feature acceptance and exclusions

**Verified:** 2026-09-10 against committed revision `ca55e7d8`; Renderer post-processing, display settings, shader, capture, viewport, and package paths were inspected as source only; concurrent user-owned dirty paths remain outside this gate evidence

**Current readiness:** **0/100** — discovery adds no implementation or evidence credit. See [Current Feature Readiness](../../../../../../../../Acceptance/CurrentReadiness.md#renderer).

Chromatic aberration is visually simple but semantically easy to fake. A feature claim needs an authored lens model, stable units, bounded sampling, explicit order, and exact identity. Colored fringes from reconstruction, filtering, motion, or encoding remain defects.

## Gate At A Glance

| Area | Candidate | Decision still required |
| --- | --- | --- |
| product | one optional per-view radial channel-separation effect | confirm simple RGB model rather than spectral/multi-sample simulation |
| controls | strength, normalized center, start offset | freeze ranges, defaults, units, live-edit and invalid-value behavior |
| geometry | output-resolution, aspect-aware radial displacement | freeze radius normalization, direction/sign, channel offsets, and resolution scaling |
| order | after selected tone/gamut mapping; before encoding and UI | reconcile SDR and HDR target-linear domains plus debug/capture policy |
| edge/alpha | bilinear clamp-to-edge; alpha preserved | freeze coordinate and texel-center rules and high-strength bound |
| evidence | CPU pattern, stage captures, resolutions, backends, package, cost | predeclare thresholds, fixtures, defect controls, and budgets |

The table is a candidate boundary, not permission to implement it. `CHRD-00` remains `Blocked` until every decision, experiment, support cell, budget, and oracle below has an exact reviewed disposition.

## Iteration Control Record

| Field | `CHRD-00-R1` requirement |
| --- | --- |
| gate identity | `CHRD-00-R1`; supersedes an earlier prose-only gate shape but does not supersede any accepted implementation decision |
| claimant | Renderer display feature owner with independent math, architecture, UX, and evidence review |
| source baseline | revision `ca55e7d8`, exact dirty-state boundary, inspected files/commands, and negative-capability result |
| accepted document set | exact revisions/hashes of README, Discovery, Research, Semantics, Execution Architecture, User Experience, and Plan |
| permitted work | source trace, CPU/image probes, workflow dry run, cost model, validation design, and documentation only |
| prohibited work | production state/pass/shader/UI/package changes, readiness credit, or selector advertisement |
| result vocabulary | `PASS` only when all criteria pass; otherwise `Blocked` with exact owner, missing artifact, and next experiment |
| invalidation | source-route drift; feature/support matrix change; semantic constant change; pass/domain move; budget/tolerance/oracle change; release scope change |

## Discovery Scope

Included discovery surfaces are the one radial model; public/default/per-view state; active output extent/subrect geometry; SDR/HDR target-linear placement; channel sample positions; bilinear/texel/edge/alpha policy; neutral omission; graph/shader/capture/debug/UI joins; editor/automation workflow; D3D12/Vulkan mechanism; package reachability; performance/memory; failure/recovery; and negative classification of incidental fringing.

Excluded from both discovery and first release are physical calibration, lens profiles, wavelength/spectral sensor integration, authored spectral LUTs, anamorphic models, per-channel curves, guard-band generation, local volumes/masks, history/temporal sampling, quality tiers, player-facing controls, and a generic lens-effects framework. Research may compare an excluded shape only to justify the selected boundary; comparison does not admit it.

## Decision Recording Contract

Each `CHRD-*` disposition records selected answer, alternatives rejected, source/experiment basis, affected semantic/architecture/UX/plan sections, exact constants/ranges, owner/reviewer/date, invalidation triggers, and evidence/check changes. “Follow Unity,” “looks good,” or a shader literal is not a disposition.

## Current Source Truth

At `ca55e7d8`, [`PostProcessing.cpp`](../../../../../../../../../Engine/Renderer/Private/Passes/PostProcessing/PostProcessing.cpp) applies debug replacement before presentation. [`Presentation.cpp`](../../../../../../../../../Engine/Renderer/Private/Passes/Presentation/Presentation.cpp) directly tone maps, output encodes, and publishes the final product. [`ViewportDisplaySettings.h`](../../../../../../../../../Engine/Renderer/Private/View/ViewportDisplaySettings.h) has no lens state. No chromatic shader, pass, selector, history, or editor control was found.

The current gap permits one clean output-resolution insertion. It does not establish whether SDR and HDR target-linear values can share the same sampling pass, how exact debug products bypass it, or what authored strength means.

## Blocking Decisions

| ID | Decision | Required disposition |
| --- | --- | --- |
| `CHRD-01` | bounded effect | admit one simple radial RGB separation; explicitly exclude spectral integration, calibration, anamorphic/per-channel curves, local volumes, history, guard-band expansion, and quality tiers |
| `CHRD-02` | stage domain | name SDR and HDR input/output domains and fix grade/tone/gamut -> aberration -> transfer/UI order |
| `CHRD-03` | coordinate model | freeze center space, aspect correction, radius normalization, start-offset meaning, zero-radius handling, and viewport subrect behavior |
| `CHRD-04` | displacement units | freeze strength in pixels at a 1080-line reference, scaling rule, sign/direction, maximum displacement, and behavior at zero/minimized extent |
| `CHRD-05` | channel model | freeze red/green/blue sample positions and weights; decide whether green remains at the source coordinate and whether channel order follows RGB storage rather than display primaries |
| `CHRD-06` | filtering and edge | freeze bilinear texel-center convention, clamp-to-edge behavior, alpha copy, finite policy, and bounds proof |
| `CHRD-07` | selection/state | freeze global default, per-view override, requested/resolved/active/unavailable state, persistence, live edit, and capture fields |
| `CHRD-08` | debug/UI/capture | classify every debug mode, ensure UI is never distorted, and name raw pre/post/encoded products |
| `CHRD-09` | cost and implementation shape | select compute/graphics placement, pass omission, resource reuse, GPU budget, memory ceiling, and whether later fusion may be considered |
| `CHRD-10` | evidence | freeze analytic patterns, resolutions/aspects, temporal cases, backend/package matrix, tolerances, seeded defects, and escalation triggers |

## Discovery Experiments

| Experiment | Required output |
| --- | --- |
| `CHR-EXP-01` stage/domain trace | current/target resource diagram covering grading, SDR/HDR tone/gamut, aberration, encoding, debug, UI, capture, and publication |
| `CHR-EXP-02` model comparison | CPU images/tables comparing three-sample RGB and multi-sample spectral precedent at low/high strengths, edges, high-frequency patterns, and cost |
| `CHR-EXP-03` unit/geometry study | displacement tables across 720p, 1080p, 1440p, 4K, ultrawide, portrait, subrect, center, axes, corners, and resize |
| `CHR-EXP-04` boundary study | edge/alpha/finite results for clamp, high strength, zero extent, missing/stale input, and odd dimensions |
| `CHR-EXP-05` workflow walkthrough | reviewed control labels, identity/reset, two-view isolation, debug bypass, capture metadata, invalid input, and package route |
| `CHR-EXP-06` evidence dry run | checks that fail for swapped channels, wrong resolution scale, missing aspect correction, wrong stage, wrap sampling, alpha mutation, and silent disable |

## Risks

| ID | Cause, event, consequence | Prevention/detection | Contingency and retirement |
| --- | --- | --- | --- |
| `RISK-CHR-01` | strength is normalized or resolution-relative inconsistently, so authored appearance changes with resolution | `CHRD-04`; matched-pattern displacement oracle | block active state; semantic owner retires after cross-resolution proof |
| `RISK-CHR-02` | wrong stage distorts UI, encodes samples twice, or creates HDR/SDR divergence | `CHRD-02/08`; raw stage captures and topology audit | remove pass edge and block `DSP-6`; display owner retires after both target domains pass |
| `RISK-CHR-03` | high-strength coordinates wrap or leave resource bounds | explicit clamp and maximum displacement; edge/corner defect cases | identity/refusal for invalid state; pass owner retires after native validation and bounds checks |
| `RISK-CHR-04` | ordinary color fringing is mislabeled intentional | selector/capture metadata plus zero-identity and exclusion audit | feature remains unavailable; acceptance owner retires after artifact classification review |
| `RISK-CHR-05` | multi-sample or framework complexity grows beyond a small lens pass | `CHRD-01/09`; public-surface and scoped-diff review | select the simple model or re-scope before code; Renderer owner retires at Stage 0 |
| `RISK-CHR-06` | a three-sample approximation produces unacceptable discontinuities/aliasing at the admitted strength | compare simple and multisample reference outputs on high-frequency patterns before model selection | lower the admitted bound or return to product scope; semantic/product owner retires after thresholded artifact review |
| `RISK-CHR-07` | viewport subrect, off-center lens center, odd extent, or DPI/window data is confused with active output geometry | explicit coordinate manifest and subrect/resize cases | reject active state for incoherent extent; View/pass owner retires after analytic coordinate evidence |
| `RISK-CHR-08` | live settings/reset or View switching leaves stale active/result state despite no persistent GPU resource | immutable request/frame generation and interleaved two-view check | omit affected pass and republish coherent state; View owner retires after state/action evidence |
| `RISK-CHR-09` | later fusion changes stage, sampler, precision, zero omission, or capture meaning | preserve semantic/product IDs and compare fused/unfused raw products plus graph traces | retain the distinct pass; pass owner retires only after equivalence and measured benefit |
| `RISK-CHR-10` | evidence calls intentional fringes correct while reconstruction/filtering defects remain | selector-off baseline, artifact provenance, known pattern, and stage/product manifest | verdict is inconclusive; acceptance owner retires after controlled-on/off classification |

## Discovery Acceptance

| ID | Pass criterion | Required evidence |
| --- | --- | --- |
| `AC-CHRD-01` | `CHRD-01` through `10` have reviewed dispositions and no implementation-shaping unknown remains | signed decision/alternative/invalidation table |
| `AC-CHRD-02` | every current/target product has one domain, format, extent/subrect, owner, and ordering edge across SDR/HDR/debug/UI/capture | `CHR-EXP-01` source-linked diagram and product ledger |
| `AC-CHRD-03` | the selected model, controls, geometry, units, channels, filter, edge, alpha, finite behavior, and neutral rule are independently executable | accepted Semantics revision plus `CHR-EXP-02/03/04` tables/images |
| `AC-CHRD-04` | settings/View/graph/shader/capture/editor/package owners define identity, lifetime, publication, resize, failure, and deletion without duplicate state | architecture owner/producer/consumer/lifetime ledger |
| `AC-CHRD-05` | every admitted user state and action is reachable, truthful, recoverable, and automation-equivalent; exclusions remain absent | accepted UX revision and `CHR-EXP-05` dry run |
| `AC-CHRD-06` | every `AC-CHR-*`, `FM-CHR-*`, `RISK-CHR-*`, profile/backend/product cell, and exclusion maps to a predeclared check | no-orphan traceability and evidence matrix |
| `AC-CHRD-07` | cost/resource budgets cover active/neutral paths, 1080p/4K, resize, captures, pipeline variants, and optional future fusion | measured/modelled budget sheet with escalation triggers |
| `AC-CHRD-08` | evidence detects swapped channels, wrong scale/aspect/stage, wrap, alpha mutation, stale state, silent disable, and incidental-fringe misclassification | `CHR-EXP-06` seeded-defect trial |
| `AC-CHRD-09` | every plan stage has prerequisites, estimates, non-goals, owned output, deletion list, stop conditions, and falsifiable exit | accepted Plan and stage dry-run record |
| `AC-CHRD-10` | independent reviewers can reproduce a hand case and determine active versus incidental fringing without private explanation | exact-revision math, architecture, UX, and evidence review record |

## Discovery Failure Modes

| ID | Controlled failure | Required safe result | Detecting checks |
| --- | --- | --- | --- |
| `FM-CHRD-01` | model/domain/unit/channel/filter constant is left unresolved and selected in code/prompt | production remains unauthorized; return choice to Discovery and invalidate dependents | `CHK-CHRD-01/03/08` |
| `FM-CHRD-02` | evidence uses visual approval without analytic coordinates/filter values or stage identity | result is `Inconclusive`; add independent numeric/raw product oracle | `CHK-CHRD-03/04/06` |
| `FM-CHRD-03` | an included control/profile/backend/debug/package path lacks state, failure, owner, and evidence | no-orphan gate fails and surface remains unreachable | `CHK-CHRD-02/07/09` |
| `FM-CHRD-04` | simple-versus-spectral comparison has no declared quality/cost question or threshold | `CHRD-01/09` stay blocked; no model is selected by preference | `CHK-CHRD-03/10` |
| `FM-CHRD-05` | zero/invalid/resize/minimize behavior is described only as implementation detail | discovery remains blocked because product truth and safety are undefined | `CHK-CHRD-04/05/08` |
| `FM-CHRD-06` | ordinary reconstruction/filtering fringing can satisfy the planned visual check while the effect is off | artifact-classification claim fails; introduce on/off and product-lineage controls | `CHK-CHRD-06/07` |

## Check Design Ledger

Each Stage-0 check records initial state, action/injected defect, independent oracle, matrix, threshold, artifact, maximum work, cleanup, and escalation.

| ID | Smallest falsifier | Oracle/artifact | Fails when |
| --- | --- | --- | --- |
| `CHK-CHRD-01` | scan dossier/prompts for unresolved choice leakage | `CHRD-*` disposition and reference map | any implementation constant/rule is missing or silently chosen |
| `CHK-CHRD-02` | enumerate feature/control/profile/backend/product/exclusion statements | no-orphan cross-document ledger | an admitted/reachable cell lacks one owner or proof route |
| `CHK-CHRD-03` | compare three-sample and multi-sample precedents on declared patterns/strengths | CPU coordinates/images, quality metric, sample/cost table | model choice is not falsifiable or exceeds quality/cost boundary |
| `CHK-CHRD-04` | calculate centers/axes/diagonals/corners/edges for all extents/aspects/subrects | independent binary64 coordinate/filter table | scale/aspect/radius/texel/edge/alpha alternative remains ambiguous |
| `CHK-CHRD-05` | exercise neutral, invalid, zero-extent, resize, and stale frame-state transitions | state/action/generation table | work executes at neutral or incoherent state can become active |
| `CHK-CHRD-06` | trace tone/gamut, lens, encoding, debug, UI, capture products with sentinels | graph/domain/alpha/product manifest | wrong-stage, UI/alpha distortion, or accidental fringe can pass |
| `CHK-CHRD-07` | enumerate selector/build/shader/package/capture reachability and exclusions | source/package/release matrix | undeclared or excluded behavior is reachable/advertised |
| `CHK-CHRD-08` | dry-run each plan prompt against missing prerequisites and faults | reviewer transcript and stop-condition map | executor must invent policy or crosses stage boundary |
| `CHK-CHRD-09` | map every risk/failure/criterion to a controlled negative and check | zero-orphan machine-readable/reviewed mapping | any claim lacks prevention/detection/contingency/retirement evidence |
| `CHK-CHRD-10` | independent math, architecture, UX, and evidence review | named corrections and exact-revision dispositions | result depends on private explanation or unchecked visual preference |

## Required `CHRD-00-R1` Evidence Package

The gate report retains:

1. exact revision, dirty-state boundary, inspected commands/files, current-route trace, and negative capability result;
2. all `CHRD-01` through `10` dispositions, alternatives, rationale, reviewers, and invalidation triggers;
3. accepted feature/support/reachability/exclusion matrices and exact companion-document revisions;
4. simple-versus-spectral comparison, frozen geometry/channel/filter hand cases, and quality/cost decision;
5. owner/producer/consumer/state/lifetime/extent/failure/deletion ledger and end-to-end sequence;
6. first-use, reset, invalid, two-view, resize/minimize, debug/UI/capture, package, accessibility, and automation dry runs;
7. numeric cost/memory/artifact budgets, tolerances, fixtures, seeded defects, and oracle independence record;
8. stage estimates, no-orphan mapping, exact stop/escalation rules, rights/provenance disposition, and independent review;
9. exact checks run, outputs, limitations/unavailable checks, and final `PASS` or `Blocked` verdict.

## Gate Decision

The gate is currently **Blocked**. A future `PASS` requires all discovery criteria conjunctively, names the exact accepted dossier revisions, and authorizes only Stage 1 of [Plan](Plan.md)—not feature implementation, runtime support, or `FCR-REN-25`.
