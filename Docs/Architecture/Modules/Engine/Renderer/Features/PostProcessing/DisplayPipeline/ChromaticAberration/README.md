# Renderer Chromatic Aberration

**Status:** first-release target feature dossier plus current source-backed absence; `CHRD-00` and production implementation are `Blocked`

**Responsibility:** define the bounded lens-effect product, current absence, target surface, acceptance, failures, checks, and definition of done for `FCR-REN-25`

**Authority boundary:** [Discovery](Discovery.md) freezes the model; [Research](Research.md) owns precedent; [Semantics](Semantics.md) owns coordinate/filter rules; [Execution Architecture](ExecutionArchitecture.md) owns state and execution; [User Experience](UserExperience.md) owns the development workflow; [Plan](Plan.md) owns delivery; this dossier owns feature acceptance; candidate results remain in `FCR-REN-25`

**Verified:** 2026-09-10 against committed revision `ca55e7d8`; current Renderer/shader/View/debug/capture/editor/package paths were inspected as source only; concurrent user-owned dirty paths were not treated as committed proof

**Scope:** `REN-POST-12`; intentional wavelength/channel-dependent lens distortion as a post-processing effect

**Parent family:** [Display Pipeline](../README.md)

**First-release admission:** `FCR-REN-25`; implementation phase [`DSP-6`](../../../../FirstRelease/DisplayAndReconstruction.md#dsp-6--chromatic-aberration)

**Current readiness:** **0/100** — admitted and `Blocked`; no lens/channel-distortion controls, pass, shader, selector, or editor route was found. See [Current Feature Readiness](../../../../../../../../Acceptance/CurrentReadiness.md#renderer).

## At A Glance

| Question | Current answer |
| --- | --- |
| Is there an intentional chromatic-aberration effect? | No selector, setting, pass, shader, lens model, or editor control exists. |
| Are colored reconstruction/filtering fringes the feature? | No. They are artifacts unless produced by a named lens model with authored controls. |
| Where does the target effect live? | After target-specific tone/gamut mapping and before final encoding/UI, on the output-resolution target-linear image. |
| What is the main design cost? | The effect needs bounded edge sampling, stable center/aspect behavior, extra texture reads, and resolution-independent tuning without contaminating alpha or UI. |

The admitted implementation must make zero strength an exact identity transform and keep artifacts from being mislabeled as deliberate output. The retained negative boundary protects both current feature claims and image-quality bug classification until that implementation exists.

## Outcome And Bounded Claim

The admitted outcome is one inexpensive, deliberately authored per-view radial channel-separation effect with stable reference-height units, exact zero omission, bounded sampling, target-linear placement, and reproducible state/capture identity on both Renderer backends. It may be called implemented only when the analytic coordinate/filter oracle, stage products, lifecycle/workflow, package route, performance budget, and artifact-classification checks pass for the same candidate.

The claim excludes physical lens calibration, wavelength transport, spectral sensor response, real-world lens profiles, anamorphic behavior, local volumes, temporal stabilization, guard-band reconstruction, user-authored per-channel curves, and quality tiers. Vendor/engine precedent informs discovery but cannot prove Sparkle's model.

## Feature Set

| ID | Surface | First-release disposition | Proof owner |
| --- | --- | --- | --- |
| `CHR-FS-01` | global default plus per-view override | included | settings/View state and `AC-CHR-05` |
| `CHR-FS-02` | one strength in pixels at a 1080-line reference | candidate; blocked by `CHRD-04` | semantics and `AC-CHR-02/03` |
| `CHR-FS-03` | normalized center and start offset | included after exact geometry freeze | semantics and `AC-CHR-02` |
| `CHR-FS-04` | fixed three-channel radial sampling | candidate; compare against AMD/Unity spectral precedent | discovery, semantics, `AC-CHR-02` |
| `CHR-FS-05` | target-linear pass after tone/gamut and before encoding/UI | candidate; domain cells blocked by `CHRD-02` | graph/product evidence and `AC-CHR-04` |
| `CHR-FS-06` | bilinear clamp-to-edge and alpha copy | included after texel-center/bounds proof | semantics and `AC-CHR-02/04/06` |
| `CHR-FS-07` | exact neutral pass/resource omission | included | graph/resource trace and `AC-CHR-01` |
| `CHR-FS-08` | editor controls, reset, truthful requested/active state | included for DevelopmentEditor | UX and `AC-CHR-05/06` |
| `CHR-FS-09` | raw pre/post plus encoded/UI capture lineage | included for evidence/support | capture owner and `AC-CHR-04/07` |
| `CHR-FS-10` | D3D12/Vulkan and admitted packaged product | included only where release matrix admits the product | `AC-CHR-07` |
| `CHR-FS-11` | spectral LUT/multisample, physical profiles, anamorphic, guard band | excluded | selector/source/package absence audit |
| `CHR-FS-12` | local volumes, masks, history, quality tiers, player controls | excluded | selector/state/workflow absence audit |

## Product And Support Matrix

| Product / mode | Authored controls | Runtime effect | Required result |
| --- | --- | --- | --- |
| DevelopmentEditor D3D12 | global/default and per-view | target when active | full semantic, workflow, capture, failure, and cost evidence |
| DevelopmentEditor Vulkan | same contract | target when active | same plus raw backend comparison and native validation |
| packaged Runtime D3D12 | cooked/configured values only if product scope admits them | target | dependency/configuration reachability and clean-machine result |
| packaged Runtime Vulkan | same if release backend is admitted | target | package plus backend evidence |
| SDR / HDR output | one effect model over explicitly named target-linear domains | separate matrix cells | matched coordinate behavior and correct stage/product lineage |
| exact debug views | no artistic distortion unless Debug Views explicitly owns an exception | bypass target | selector/topology/capture proof |
| UI | no distortion | downstream composition | alpha/UI sentinel capture |
| excluded physical/spectral/volume/history surfaces | no controls/state/shaders/assets/package claims | absent | negative reachability audit |

## Start Here

| Question | Owning document |
| --- | --- |
| What must the feature provide and prove? | This dossier |
| Which model, unit, order, and budget choices remain blocked? | [Discovery](Discovery.md) |
| What do mature engine implementations establish or not establish? | [Research](Research.md) |
| What are the exact candidate coordinates, displacement, filtering, and identity rules? | [Semantics](Semantics.md) |
| Who owns settings, View state, pass placement, failure, and cost? | [Execution Architecture](ExecutionArchitecture.md) |
| What should first use, state truth, reset, failure, capture, and automation feel like? | [User Experience](UserExperience.md) |
| In what order can it be delivered? | [Plan](Plan.md) |
| What did a candidate prove? | [`FCR-REN-25`](../../../../../../../../Acceptance/FeatureCompletionReports.md#initial-completion-report-registry) and retained evidence |

## Current Capability

Chromatic aberration was not found. Sparkle has no per-view lens/aberration settings, selector, pass, shader, radial or spectral distortion model, center/falloff controls, edge sampling policy, history, diagnostic mode, or editor authoring surface.

Color fringes caused by reconstruction, filtering, motion, texture sampling, or display encoding are artifacts, not an implemented chromatic-aberration feature. No neighboring post stage may be advertised as this effect.

## First-Release Target Contract

The first-release candidate is one View-owned, output-resolution lens-fringe pass after target tone/gamut mapping and grading but before output encoding and UI. It uses normalized center and start offset plus a strength expressed in pixels at a 1080-line reference height; scaling by output height is intended to keep authored displacement stable across resolution. Exact radius normalization, falloff, channel offsets, domains, and bounds remain blocked by [`CHRD-00`](Discovery.md). [Research](Research.md) records the reviewed product and source precedents; none is a shader implementation to copy.

The admitted scope must define and implement:

- the fixed post-tone/pre-encode position, with raw input/output capture identity and UI excluded from distortion;
- the radial RGB channel-offset model, reference-height units, viewport center/aspect behavior, bilinear sampling, bounded edge clamp, alpha preservation, and one quality mode;
- per-camera/per-viewport settings, defaults, serialization, editor controls, requested/active diagnostics, and disable/identity behavior;
- extent/resize/history behavior, compute or graphics pass ownership, backend requirements, cost, and controlled failure;
- reference patterns for zero-strength identity, center stability, radial symmetry, edge behavior, temporal stability, and resolution independence.

Physical lens calibration, spectral rendering, anamorphic models, per-channel author curves, guard-band expansion, history, local volumes, and multiple quality modes are non-goals for `v0.1.0`. Until implementation exists, the admitted feature remains unavailable and `Blocked`, not Experimental.

## Design Decisions And Tradeoffs

| Candidate direction | Benefit | Cost/constraint | Decision owner |
| --- | --- | --- | --- |
| fixed three-channel model | predictable three filtered reads and a small analyzable contract | less spectral smoothness/physical plausibility than multi-sample models | `CHRD-01/05/09` |
| reference-height pixel strength | author intent can be tested across resolution | requires exact active-extent scaling and clear UI units | `CHRD-04` |
| aspect-aware radial coordinates | stable behavior across ultrawide, portrait, subrect, off-center views | more semantic cases than naive normalized radius | `CHRD-03` |
| target-linear placement | leaves UI sharp and avoids sampling encoded values | SDR/HDR domains must be named and separately evidenced | `CHRD-02/08` |
| clamp-to-edge without guard band | bounded, simple first release | high strengths duplicate edge pixels; maximum displacement must be bounded | `CHRD-06` |
| distinct initial pass | preserves raw products and defect localization | adds one active output-resolution read/write resource route | `CHRD-09` |
| exact zero omission | truthful cost and identity | neutral predicate must be centralized across settings/View/graph | `CHRD-07/09` |

These are discovery candidates. A changed choice must update semantics, architecture, UX, plan, fixtures, budgets, and acceptance mapping together.

## First-Release Acceptance Criteria

- `AC-CHR-01` — zero strength is an exact identity and omits the pass; disabled state does not allocate history or persistent GPU data.
- `AC-CHR-02` — the radial channel-offset equation matches a CPU reference for center, axes, diagonals, aspect ratios, start offsets, strengths, and edge coordinates within declared filtering tolerance.
- `AC-CHR-03` — strength scales from the 1080-line reference to output height, so matched normalized patterns have the same authored displacement within tolerance at every admitted resolution.
- `AC-CHR-04` — the pass consumes tone-mapped display-linear color after grading and before encoding/UI exactly once; alpha and viewport/product identity are preserved.
- `AC-CHR-05` — per-view requested/resolved/active state, persistence, live edit, two-viewport isolation, resize/DPI, capture metadata, and invalid values behave deterministically.
- `AC-CHR-06` — non-finite, negative/out-of-range, zero-extent, missing-input, stale-generation, and resource/pipeline failures reject or produce the documented identity safe state without stale or partial output.
- `AC-CHR-07` — D3D12/Vulkan raw results, temporal stability, package operation, pass cost, and memory satisfy declared release tolerances and budgets.
- `AC-CHR-08` — ordinary reconstruction/filtering fringes remain defects; excluded physical/spectral/local-volume/guard-band/history variants are not claimed.

## First-Release Failure Modes And Checks

| Failure ID | Controlled setup | Required safe behavior | Check |
| --- | --- | --- | --- |
| `FM-CHR-01` | invalid/non-finite strength, center, or start offset | reject or clamp exactly as documented before active publication; identify the field | `CHK-CHR-01`, `CHK-CHR-04` |
| `FM-CHR-02` | zero extent, missing input, stale generation, or resize during execution | no out-of-bounds/stale sampling; identity/refusal and rebuild are explicit | `CHK-CHR-03`, `CHK-CHR-04` |
| `FM-CHR-03` | effect runs in the wrong color domain/order or distorts UI/alpha | topology/raw-product oracle fails the candidate | `CHK-CHR-02` |
| `FM-CHR-04` | high strength reaches image edges | all sample coordinates remain bounded and deterministic; no NaN, wrap, or memory fault | `CHK-CHR-01`, `CHK-CHR-03` |
| `FM-CHR-05` | backend/package result or cost differs materially | block rather than silently disable or relabel artifacts as the feature | `CHK-CHR-05` |

| Check | Exercise and oracle | Covers |
| --- | --- | --- |
| `CHK-CHR-01` | CPU-versus-shader radial pattern over centers, axes, diagonals, strengths, offsets, aspects, and edges | `AC-CHR-01`, `AC-CHR-02`; `FM-CHR-01`, `FM-CHR-04` |
| `CHK-CHR-02` | graph/domain audit and raw pre/post/encoded/UI captures with alpha sentinel | `AC-CHR-04`; `FM-CHR-03` |
| `CHK-CHR-03` | matched patterns at admitted resolutions plus resize, DPI, edge, zero-extent, and temporal camera cases | `AC-CHR-03`, `AC-CHR-07`; `FM-CHR-02`, `FM-CHR-04` |
| `CHK-CHR-04` | settings round trip, two viewports, invalid input, missing resource/pipeline, generation failure and recovery | `AC-CHR-05`, `AC-CHR-06`; `FM-CHR-01`, `FM-CHR-02` |
| `CHK-CHR-05` | paired D3D12/Vulkan raw comparison, package route, timing/memory and artifact-classification review | `AC-CHR-07`, `AC-CHR-08`; `FM-CHR-05` |

## Cross-Document Traceability

| Surface | Discovery | Research | Semantics | Architecture / UX | Plan | Acceptance / checks | Result |
| --- | --- | --- | --- | --- | --- | --- | --- |
| model and controls | `CHRD-01/03/04/05` | `CHR-REF-AMD-01/02`, `CHR-REF-UNITY-01/02/03` | `CHR-MATH-01/02/03/07` | View request/result and control contract | Stages 0-2/3 | `AC-CHR-01/02/03/05`; `CHK-CHR-01/03/04` | `FCR-REN-25` |
| stage/domain/UI/debug | `CHRD-02/08` | `CHR-REF-AMD-03`, `CHR-REF-UNITY-02` | `CHR-MATH-05/09` | graph products and UX capture/status | Stages 2-3 | `AC-CHR-04/08`; `CHK-CHR-02/05` | `FCR-REN-25` |
| filtering/edge/extent | `CHRD-03/04/06` | source implementation comparison | `CHR-MATH-01/02/04/05/07` | immutable frame extent; error contract | Stage 2 | `AC-CHR-02/03/06`; `CHK-CHR-01/03/04` | `FCR-REN-25` |
| selection/lifecycle | `CHRD-07` | activation precedent | `CHR-MATH-06/07/08` | settings/View/graph state and reset UX | Stages 1/3 | `AC-CHR-01/05/06`; `CHK-CHR-04` | `FCR-REN-25` |
| backend/package/cost | `CHRD-09/10` | evidence and cost precedent | fixed semantic contract | support/evidence and reachability matrices | Stage 4 | `AC-CHR-07/08`; `CHK-CHR-03/05` | `FCR-REN-25` |
| exclusions/artifact classification | `CHRD-01/08/10` | rejected-transfer ledger | no admitted rules | absent controls/assets/history; truthful labels | all stages | `AC-CHR-08`; `CHK-CHR-05/NEG-01` | `FCR-REN-25` |

## Evidence And Source Audit

- `REN-E27` owns the negative source/build/selector/pass/shader/asset/editor/documentation audit for chromatic aberration.
- No runtime test is implied by this source-only absence finding.
- Adjacent source routes inspected: [`PostProcessing.cpp`](../../../../../../../../../Engine/Renderer/Private/Passes/PostProcessing/PostProcessing.cpp), [`Presentation.cpp`](../../../../../../../../../Engine/Renderer/Private/Passes/Presentation/Presentation.cpp), and [`ViewportDisplaySettings.h`](../../../../../../../../../Engine/Renderer/Private/View/ViewportDisplaySettings.h).

### Current Negative Acceptance

- `AC-CHR-NEG-01` — no selector, view/camera setting, lens model, shader/pass, history, diagnostic, editor control, or package claim advertises chromatic aberration.
- `AC-CHR-NEG-02` — reconstruction, filtering, motion, sampling, and encoding fringes remain classified as defects/artifacts rather than intentional feature output.

`FM-CHR-NEG-01` occurs when a reachable aberration-like control/path lacks the complete owner/result contract or an artifact is advertised as support. `CHK-CHR-NEG-01`/`REN-E27` covers `AC-CHR-NEG-01`, `AC-CHR-NEG-02`, and `FM-CHR-NEG-01` by auditing source, CMake, shaders, settings, frame graph, view/editor UI, diagnostics, packages, and documentation; any unmatched result fails the negative contract.

## Definition Of Done

Chromatic aberration is complete only when `CHRD-00` passed before implementation, the accepted model and units drive one per-view pass on every admitted backend/domain, zero is exact no-work identity, all `AC-CHR-01` through `08` pass, every `FM-CHR-*` and risk has controlled detecting evidence, debug/UI/capture/package boundaries are truthful, excluded spectral/volume/history/quality surfaces remain unreachable, temporary probes are removed, and the acceptance owner records `FCR-REN-25 PASS` against one immutable candidate.
