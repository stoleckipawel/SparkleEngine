# Renderer Color Grading

**Status:** first-release target feature dossier plus current source-backed absence; `CGRD-00` and production implementation are `Blocked`

**Responsibility:** define the bounded color-grading product, current absence, target feature set, acceptance criteria, failures, checks, and conjunctive definition of done for `FCR-REN-24`

**Authority boundary:** [Discovery](Discovery.md) freezes implementation-shaping decisions; [Research](Research.md) owns precedent; [Semantics](Semantics.md) owns math and LUT interpretation; [Execution Architecture](ExecutionArchitecture.md) owns system shape; [User Experience](UserExperience.md) owns developer workflow; [Plan](Plan.md) owns delivery order; code/build configuration owns implementation; `FCR-REN-24` owns candidate results

**Verified:** 2026-09-10 against committed revision `30597d7d`; current Renderer/shader/asset/settings/editor/package paths were re-inspected as source only; unrelated dirty work was present outside this package

**Scope:** `REN-POST-11`; artistic and technical color adjustments distinct from exposure, tone mapping, and output encoding

**Parent family:** [Display Pipeline](../README.md)

**First-release admission:** `FCR-REN-24`; implementation phase [`DSP-5`](../../../../FirstRelease/DisplayAndReconstruction.md#dsp-5--color-grading)

**Current readiness:** **0/100** — admitted and `Blocked`; no grading controls, transforms, LUT resource, pass, shader, selector, or editor route was found. See [Current Feature Readiness](../../../../../../../../Acceptance/CurrentReadiness.md#renderer).

## At A Glance

| Question | Current answer |
| --- | --- |
| Can a user author a look or technical color transform? | No grading parameters, LUT assets, import/cook path, blend stack, pass, shader, or per-view controls exist. |
| Is tone-mapper selection color grading? | No. The current operators are fixed HDR-to-display mappings, not an authored look. |
| Is Linear/sRGB output selection grading? | No. It selects transfer encoding, not color adjustment or gamut conversion. |
| What target is admitted for discovery? | One scene-referred stage after reconstruction and before target-specific tone mapping, with global controls plus one metadata-bearing 3D LUT generation owned per view; `CGRD-00` must ratify every exact semantic and lifecycle choice. |

The admitted contract is larger than a saturation slider: it must carry authored color-space metadata from asset/import/cook through a deterministic per-view transform and a measurable SDR/HDR result.

> [!IMPORTANT]
> **Current state:** `Not found`; only adjacent reconstruction, exposure, tone-mapping, output-encoding, generic texture, settings, editor, and capture routes exist.
>
> **Main limitation:** scene working space, negative/HDR math, `.cube` grammar, LUT mapping, generation lifecycle, budgets, and evidence thresholds remain unresolved under `CGRD-00`.
>
> **Evidence boundary:** this package is a target and proof contract. It provides no build, shader-cook, runtime, image-quality, performance, package, security, or release evidence.

## Outcome And Bounded Claim

Given a finite reconstructed scene-color product, an accepted per-view grade request, and—when selected—one valid cooked LUT generation, the target Renderer produces one generation-qualified graded scene-color product in the same declared scene-working space. Neutral state produces no grading pass. Invalid requested state never becomes active silently, and downstream tone mapping/output owns display conversion.

The bounded claim includes global slope/offset/power, saturation, one optional typed 3D LUT, live DevelopmentEditor authoring, packaged consumption, two-view isolation, raw-stage observability, and D3D12/Vulkan semantic parity. It excludes local blending volumes, multiple looks, 1D/shaper pipelines, curves, masks, keyframes, runtime OCIO, display calibration, and a general color-management system. Exclusion means absent or visibly rejected—not accepted as a no-op.

The feature is complete only as the conjunction of semantic correctness, transactional asset/runtime identity, author workflow, backend/profile coverage, controlled failures, bounded cost, package reachability, and candidate-bound evidence. A correct shader without a safe parser/package route is incomplete; a working LUT editor without known color meaning is incomplete.

## Feature Set

| ID | Surface | First-release disposition | Boundary or blocker |
| --- | --- | --- | --- |
| `CGR-FS-01` | exact neutral/disabled state | Included, pending `CGRD-00` | no pass, allocation, history reset, or pixel change |
| `CGR-FS-02` | per-channel slope/offset/power | Included, pending exact negative/range policy | one ordered global transform, not lift/gamma/gain aliases |
| `CGR-FS-03` | working-space saturation | Included, pending luma/range policy | coefficients derive from accepted primaries |
| `CGR-FS-04` | one optional `.cube`-derived 3D LUT | Included, pending grammar/domain/interpolation limits | typed source/cooked/runtime asset; no opaque generic texture claim |
| `CGR-FS-05` | parametric-plus-LUT composition | Included, order pending | exactly once; no blend stack |
| `CGR-FS-06` | global default and per-view override | Included, pending state/digest schema | View owns selection; residency owns immutable LUT generation |
| `CGR-FS-07` | DevelopmentEditor authoring/live update | Included, pending UX and settlement bounds | requested/resolving/active/failed truth; stale completion rejected |
| `CGR-FS-08` | packaged Runtime consumption | Included, pending package manifest | consumes cooked data only; no source parser in Runtime |
| `CGR-FS-09` | raw stage/capture diagnostics | Included, pending artifact schema | pre/post-grade remains distinct from tone/encoding/screenshot |
| `CGR-FS-10` | D3D12 and Vulkan | Included, pending executable parity | one semantic shader contract; backend result cells remain unproved |
| `CGR-FS-11` | local volumes, multi-LUT blending, curves, masks, timelines | Excluded | no selectors, serialized vocabulary, runtime graph, or claims |
| `CGR-FS-12` | OCIO runtime, display calibration, display-referred legacy LUTs | Excluded | research precedent and source import do not admit these products |

## Product And Support Matrix

| Product/profile | Request and input | Target active result | Failure/fallback truth | Current evidence |
| --- | --- | --- | --- | --- |
| DevelopmentEditor D3D12 | global/per-view parameters and optional source LUT | live generation-qualified scene grade with author diagnostics | keep last explicit good or neutral state according to `CGRD-09/11`; never label failed request active | target only |
| DevelopmentEditor Vulkan | same serialized intent and cooked/runtime LUT semantics | numerically equivalent raw graded product | same terminal categories; backend-specific reason visible | target only |
| packaged Runtime D3D12 | persisted settings plus packaged cooked LUT identity | no source parsing; active generation matches manifest | missing/corrupt bytes refuse the LUT and report exact state | target only |
| packaged Runtime Vulkan | same as D3D12 | same semantic result and package identity | same safe state and reason vocabulary | target only |
| offscreen/viewport capture | explicit pre/post-grade product request | domain/format/extent/alpha/generation manifest accompanies pixels | ambiguous or stale product is rejected | target only |
| headless source conversion | `.cube` import/cook operation only if admitted by existing tools | transactional canonical cooked output | no renderer-active claim; stable error category and no partial publish | target only |
| local-volume/multi-look/OCIO-runtime request | excluded intent | no active state | reject/omit visibly; never approximate with the global grade | excluded |

## Start Here

| Question | Owning document |
| --- | --- |
| What must the feature provide and prove? | This dossier |
| Which decisions block implementation? | [Discovery](Discovery.md) |
| Which primary sources and local seams informed the design? | [Research](Research.md) |
| What exactly do slope/offset/power, saturation, and LUT samples mean? | [Semantics](Semantics.md) |
| Who owns source, cooked, runtime, View, pass, and retirement state? | [Execution Architecture](ExecutionArchitecture.md) |
| How does a developer author, diagnose, reset, and automate a look? | [User Experience](UserExperience.md) |
| In what order can work proceed? | [Plan](Plan.md) |
| What did a candidate prove? | [`FCR-REN-24`](../../../../../../../../Acceptance/FeatureCompletionReports.md#initial-completion-report-registry) and retained evidence |

## Current Capability

Color grading was not found. Sparkle has no grading pass or shader, per-view grading settings, lift/gamma/gain or slope/offset/power controls, saturation/contrast/hue controls, 1D or 3D grading LUT asset/import/cook path, LUT blend stack, working/display gamut selection, or grading selector/debug product.

The three tone-mapper operators are fixed HDR-to-display mappings. They do not constitute an authored color-grading stack, and output Linear/sRGB encoding is a transfer-function choice rather than grading.

## First-Release Target Contract

The first-release candidate is one scene-referred grading stage after reconstruction and before target tone mapping. It owns a deterministic global parametric grade plus one optional 3D LUT generation. The exact working space, exposure relation, negative-value policy, LUT subset/layout/interpolation, and budgets remain blocked by [`CGRD-00`](Discovery.md). External precedent is cataloged in [Research](Research.md); it does not define Sparkle's types or prove its result.

The admitted scope must define and implement:

- scene-referred linear input/output, fixed placement before tone mapping, and a separately visible relationship to exposure;
- one ordered global slope/offset/power plus saturation control set and one optional `.cube`-derived 3D LUT generation, with explicit working-space metadata, trilinear interpolation, precision, and identity defaults;
- per-view overrides, requested/active state, defaults, serialization, and editor UX;
- shader/pass inputs and outputs, gamut and alpha behavior, backend requirements, diagnostics, and failure when a LUT or transform is invalid;
- numerical references, identity-transform proof, LUT edge cases, artifact fixtures, cost, and candidate release criteria.

Local volumes, multiple blended LUTs, display-referred legacy LUTs, OpenColorIO, curves, masks, and shot timelines are non-goals for `v0.1.0`. Until the implementation owners exist, documentation and UI report the admitted feature as unavailable and `Blocked`; tone-mapper choice is never a substitute.

## Design Decisions And Tradeoffs

| Candidate direction | Benefit | Cost or rejected alternative | Gate owner |
| --- | --- | --- | --- |
| scene-referred grade before target mapping | one authored look can feed SDR and HDR without duplicating display transforms | negative/HDR values and exposure relation must be explicit; display-referred LUT habits do not transfer | `CGRD-02/03` |
| narrow SOP plus saturation surface | analyzable identity/order with a small public contract | less artist breadth than lift/gamma/gain, curves, wheels, or volume stacks | `CGRD-01/04/05` |
| one typed `.cube` source to canonical cooked 3D data | reproducible parsing, package membership, hashing, limits, and diagnostics | new semantic asset/cook work; generic texture upload alone is insufficient | `CGRD-06/07/10` |
| immutable LUT generation referenced by each View | no duplicated LUT bytes and transactional live replacement | generation/publication/retirement machinery is required | `CGRD-09/10` |
| distinct grade stage before considering fusion | raw boundaries and failures are inspectable | one extra pass/resource until equivalence and cost justify fusion | `CGRD-03/12` |
| strict reject or explicit prior-good retention | failed requests cannot masquerade as the requested look | authors must see requested and active identities separately | `CGRD-09/11` |

These are candidate directions, not accepted policy. Discovery may replace them, but it must update semantics, architecture, UX, plan, and proof mapping together.

## First-Release Acceptance Criteria

- `AC-CGR-01` — zero/default parameters with no LUT are an exact identity at the declared precision, and the graph omits grading work when no non-identity state is active.
- `AC-CGR-02` — the ordered slope/offset/power and saturation transform matches a CPU reference for neutral ramps, primaries, secondaries, gray steps, negative values, and HDR values within a declared tolerance.
- `AC-CGR-03` — one admitted 3D LUT imports and cooks transactionally with working-space, dimension, encoding, interpolation, and content identity; identity and known-cell LUT fixtures match the CPU oracle.
- `AC-CGR-04` — grading is View-owned requested/resolved/active state, persists through the existing settings path, isolates two viewports, and invalidates only its own generation on edit, reload, or LUT replacement.
- `AC-CGR-05` — grading consumes reconstructed scene-referred linear color exactly once before tone mapping; exposure, tone mapping, HDR output, encoding, debug, capture, and UI boundaries remain separately identifiable.
- `AC-CGR-06` — missing, malformed, dimension-mismatched, non-finite, stale, or unsupported LUT/parameter input refuses activation or retains the last explicitly reported good generation without partial publication or silent identity substitution.
- `AC-CGR-07` — D3D12 and Vulkan decoded raw outputs agree with the same numerical oracle; candidate evidence records pass cost, memory, package membership, and clean-machine authored-setting operation.
- `AC-CGR-08` — author controls name their color domain and limits; excluded local-volume, multi-LUT, OCIO, curves, mask, and timeline behavior is neither selectable nor advertised.

## First-Release Failure Modes And Checks

| Failure ID | Controlled setup | Required safe behavior | Check |
| --- | --- | --- | --- |
| `FM-CGR-01` | malformed/non-finite parameters or LUT metadata/data | reject before active publication, identify the field/asset, and preserve a coherent prior/default generation | `CHK-CGR-01`, `CHK-CGR-03` |
| `FM-CGR-02` | LUT load/cook/upload/reload fails or completes after a newer edit | no partial/stale generation becomes active; retirement follows completion identity | `CHK-CGR-03`, `CHK-CGR-05` |
| `FM-CGR-03` | grading is ordered after tone mapping, applied twice, or shares mutable state across views | topology/identity checks fail the candidate; no plausible image counts as a pass | `CHK-CGR-02`, `CHK-CGR-04` |
| `FM-CGR-04` | unsupported/excluded control is restored or advertised | requested state reports unsupported and source/selector/package audit fails the claim | `CHK-CGR-06` |
| `FM-CGR-05` | backend/package output differs or cost/memory exceeds its budget | block the feature; do not hide the route behind silent identity fallback | `CHK-CGR-04`, `CHK-CGR-05` |

| Check | Exercise and oracle | Covers |
| --- | --- | --- |
| `CHK-CGR-01` | CPU-versus-shader parametric transform over identity, ramps, colors, negative/HDR/non-finite inputs | `AC-CGR-01`, `AC-CGR-02`; `FM-CGR-01` |
| `CHK-CGR-02` | graph/resource/domain audit and raw pre-grade/post-grade/pre-tone captures | `AC-CGR-05`; `FM-CGR-03` |
| `CHK-CGR-03` | import/cook/load of identity, known-cell, malformed, wrong-dimension, missing, and corrupt LUT fixtures | `AC-CGR-03`, `AC-CGR-06`; `FM-CGR-01`, `FM-CGR-02` |
| `CHK-CGR-04` | two viewports, settings round trip, live edit/reload, D3D12/Vulkan raw comparison | `AC-CGR-04`, `AC-CGR-07`; `FM-CGR-03`, `FM-CGR-05` |
| `CHK-CGR-05` | exact/over budget, repeated replacement, retirement, package and clean-machine route | `AC-CGR-06`, `AC-CGR-07`; `FM-CGR-02`, `FM-CGR-05` |
| `CHK-CGR-06` | selector/UI/source/package/docs audit for every included and excluded control | `AC-CGR-08`; `FM-CGR-04` |

## Cross-Document Traceability

This table is the single feature-surface mapping. Companion documents own the details named in their columns; none may introduce an unmapped surface.

| Surface | Discovery | Research | Semantics | Architecture/UX | Plan | Acceptance / failure / check | Result |
| --- | --- | --- | --- | --- | --- | --- | --- |
| neutral and parameter grade | `CGRD-01` through `05` | `CGR-REF-OCIO-01`, `CGR-REF-FIL-01` | `CGR-MATH-01` through `04` | View request/digest; neutral omission and reset UX | Stages 1-2 | `AC-CGR-01/02`; `FM-CGR-01/03`; `CHK-CGR-01/02/04` | `FCR-REN-24` |
| `.cube` source and canonical cook | `CGRD-06/07/10` | `CGR-REF-OCIO-02/03`, `CGR-REF-SEC-01`, `CGR-REF-UNITY-01` | `CGR-MATH-05/06` | asset/cook owner; validation and error UX | Stage 3 | `AC-CGR-03/06`; `FM-CGR-01/02`; `CHK-CGR-03/05` | `FCR-REN-24` |
| runtime LUT generation/application | `CGRD-07` through `10` | `CGR-REF-FIL-01/02`, `CGR-REF-UNITY-02` | `CGR-MATH-05/07/09` | residency/View generation and retirement; live-edit truth | Stage 4 | `AC-CGR-03/04/06/07`; `FM-CGR-02/03/05`; `CHK-CGR-03/04/05` | `FCR-REN-24` |
| stage/domain/SDR-HDR composition | `CGRD-02/03/08` | cross-source comparison | `CGR-MATH-01/07/10` | graph topology and raw product labels | Stages 2/4 | `AC-CGR-02/05/07`; `FM-CGR-03/05`; `CHK-CGR-01/02/04` | `FCR-REN-24` |
| authoring, persistence, two-view state | `CGRD-09/11` | View/activation precedent | `CGR-MATH-03/09` | generation/state owner and full experience contract | Stage 5 | `AC-CGR-04/06/08`; `FM-CGR-02/04`; `CHK-CGR-04/06` | `FCR-REN-24` |
| backend, capture, package, cost | `CGRD-10/12` | evidence precedent only | `CGR-MATH-08/10` | product/artifact/package identity | Stage 6 | `AC-CGR-05/07/08`; `FM-CGR-05`; `CHK-CGR-02/04/05/06` | `FCR-REN-24` |
| exclusions | `CGRD-01` | rejected-transfer ledger | no semantic rules | absent selectors/types/workflows | all stages preserve | `AC-CGR-08`; `FM-CGR-04`; `CHK-CGR-06` | `FCR-REN-24` |

## Evidence And Source Audit

- `REN-E26` owns the negative source/build/selector/pass/shader/asset/editor/documentation audit for color grading.
- No runtime test is implied by this source-only absence finding.
- Adjacent source routes inspected: [`PostProcessing.cpp`](../../../../../../../../../Engine/Renderer/Private/Passes/PostProcessing/PostProcessing.cpp), [`Presentation.cpp`](../../../../../../../../../Engine/Renderer/Private/Passes/Presentation/Presentation.cpp), and [`EngineRenderingDisplayTypes.h`](../../../../../../../../../Engine/Renderer/Public/Settings/EngineRenderingDisplayTypes.h).

### Current Negative Acceptance

- `AC-CGR-NEG-01` — no selector, per-view setting, asset/import/cook type, LUT, shader/pass, debug product, or editor control advertises color grading.
- `AC-CGR-NEG-02` — tone-mapper and output-encoding choices remain labeled as fixed display mapping/transfer, not an artistic or technical grading stack.

`FM-CGR-NEG-01` occurs when any reachable grading-like vocabulary lacks a real owner/result or when a neighboring transform is mislabeled. `CHK-CGR-NEG-01`/`REN-E26` covers `AC-CGR-NEG-01`, `AC-CGR-NEG-02`, and `FM-CGR-NEG-01` by searching source, build membership, shaders, settings, assets/tools, editor UI, package surfaces, and documentation; any unmatched result fails the negative contract and requires a new current/target dossier before advertisement.

## Definition Of Done

Color grading is complete only when `CGRD-00` passed before implementation, every included control and `.cube` behavior conforms to the accepted semantic revision, the one View/residency/pass ownership path is active on every admitted backend/profile, all `AC-CGR-01` through `08` pass, every `FM-CGR-*` and material risk has controlled detecting evidence, package and authoring journeys pass, excluded surfaces remain unreachable, temporary probes are removed, and the acceptance owner records `FCR-REN-24 PASS` against one immutable candidate. Partial stages do not average into acceptance.
