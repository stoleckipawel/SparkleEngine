# Chromatic Aberration Discovery

**Status:** acceptance contract; `CHRD-00` is `Blocked`, and production implementation is not authorized

**Responsibility:** freeze the first-release lens model, units, color/order boundary, state, edge behavior, budgets, workflow, and evidence protocol before `DSP-6`

**Authority boundary:** [Research](Research.md) provides precedent; [Semantics](Semantics.md) and [Execution Architecture](ExecutionArchitecture.md) are design candidates; [Plan](Plan.md) is conditional delivery order; [README](README.md) owns feature acceptance and exclusions

**Verified:** 2026-09-10 against committed revision `669637cf`; Renderer post-processing, display settings, shader, capture, viewport, and package paths were inspected as source only

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

## Current Source Truth

At `669637cf`, [`PostProcessing.cpp`](../../../../../../../../../Engine/Renderer/Private/Passes/PostProcessing/PostProcessing.cpp) applies debug replacement before presentation. [`Presentation.cpp`](../../../../../../../../../Engine/Renderer/Private/Passes/Presentation/Presentation.cpp) directly tone maps, output encodes, and publishes the final product. [`ViewportDisplaySettings.h`](../../../../../../../../../Engine/Renderer/Private/View/ViewportDisplaySettings.h) has no lens state. No chromatic shader, pass, selector, history, or editor control was found.

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

## Discovery Acceptance

`CHRD-00` passes only when `CHRD-01` through `10` are dispositioned, all six experiments have reviewed results, [Semantics](Semantics.md) can be implemented without choosing units or equations, [Execution Architecture](ExecutionArchitecture.md) has one owner per state/lifetime/failure, and every `AC-CHR-*`, `FM-CHR-*`, and risk maps to a predeclared defect-detecting check and budget.

`FM-CHRD-01` is an unresolved model/domain/unit choice hidden in code. `FM-CHRD-02` is a visually judged check without an analytic displacement or stage oracle. `FM-CHRD-03` is any included control/profile/backend/debug path without selection, failure, and evidence coverage. Any occurrence keeps the gate `Blocked`.

The gate is currently **Blocked**. A future `PASS` authorizes only Stage 1 of [Plan](Plan.md), not the feature verdict.

