# Color Grading Discovery

**Status:** acceptance contract; `CGRD-00` is `Blocked`, and production implementation is not authorized

**Responsibility:** freeze the first-release color domain, transform semantics, LUT contract, ownership, experience, budgets, and evidence protocol before `DSP-5` implementation begins

**Authority boundary:** [Research](Research.md) supplies precedent; [Semantics](Semantics.md), [Execution Architecture](ExecutionArchitecture.md), and [User Experience](UserExperience.md) are conditional design candidates; [Plan](Plan.md) orders work only after this gate passes; [README](README.md) owns feature acceptance; code and build configuration own implementation

**Verified:** 2026-09-10 against committed revision `669637cf`; current Renderer, shader, texture-cooking, settings, editor, and package routes were inspected as source only

**Current readiness:** **0/100** — discovery adds no implementation, verification, delivery, or adoption credit. See [Current Feature Readiness](../../../../../../../../Acceptance/CurrentReadiness.md#renderer).

Color grading cannot be reduced to adding a saturation value and a texture sample. The feature changes the meaning of scene color, introduces authored color data, crosses source-to-cooked asset identity, and must compose with exposure, tone mapping, debug views, SDR, and HDR. `CGRD-00` exists to keep those choices out of an implementation prompt.

## Gate At A Glance

| Area | Current finding | Decision required |
| --- | --- | --- |
| Existing route | no grading control, pass, shader, LUT asset, or editor route exists | confirm the clean insertion seam and deleted/superseded vocabulary |
| Working domain | current scene color is described as scene-linear, but working primaries and exposure/grade order are not an executable contract | freeze primaries, white convention, units, negative policy, and exposure relationship |
| Parametric grade | slope/offset/power plus saturation is admitted | freeze exact order, clamp/signed-power behavior, luma weights, ranges, and identity |
| LUT | one optional `.cube`-derived 3D LUT is admitted | freeze accepted grammar, axis/order, domain mapping, dimension, precision, interpolation, and invalid-data behavior |
| Ownership | target state is View-owned | freeze global defaults, per-view override, generation identity, persistence, and retirement |
| Product route | DevelopmentEditor authoring and packaged consumption are required | freeze first use, error state, automation equivalence, package policy, and support output |
| Evidence | `AC-CGR-*` exists, but thresholds and fixtures are not frozen | predeclare numeric, visual, backend, package, memory, and cost oracles |

## Current Source Truth

At `669637cf`, [`PostProcessing.cpp`](../../../../../../../../../Engine/Renderer/Private/Passes/PostProcessing/PostProcessing.cpp) resolves/upscales scene color, applies debug replacement, and calls presentation. [`Presentation.cpp`](../../../../../../../../../Engine/Renderer/Private/Passes/Presentation/Presentation.cpp) dispatches tone mapping and then output encoding. [`EngineRenderingDisplayTypes.h`](../../../../../../../../../Engine/Renderer/Public/Settings/EngineRenderingDisplayTypes.h) defines tone, exposure, and output-encoding enums only. The TextureCooker accepts raster/HDR/EXR texture sources but contains no `.cube` parser or grading-specific cooked contract.

This proves a source-backed absence and identifies possible extension points. It does not prove that the current color labels are colorimetrically complete, that a LUT may safely reuse the generic texture route, or that any proposed shader will build or run.

## Blocking Decisions

Every row must have an explicit disposition, reviewer, rationale, and invalidation trigger. A convenient implementation default is not a decision.

| ID | Decision | Required disposition |
| --- | --- | --- |
| `CGRD-01` | Bounded product | confirm global slope/offset/power, saturation, and one optional 3D LUT; confirm all local-volume, curve, mask, timeline, multi-LUT, OCIO-runtime, and display-referred look behavior remains excluded |
| `CGRD-02` | Working color space | name scene-color primaries, white point, numeric scale, legal negative/HDR range, and whether values are pre-exposed or exposure-neutral at the grading boundary |
| `CGRD-03` | Stage order | freeze reconstruction -> exposure relationship -> parametric grade -> LUT -> target tone/gamut mapping, or approve another exact order with analytic reasons |
| `CGRD-04` | SOP semantics | select clamp or no-clamp behavior, define power for negative bases, component ranges, invalid-value behavior, and CPU/shader precision |
| `CGRD-05` | Saturation semantics | freeze luminance coefficients in the declared working space, extrapolation policy, operation order, and permitted range |
| `CGRD-06` | `.cube` grammar | define accepted directives and comments, reject 1D/mixed tables unless admitted, freeze RGB sample ordering, duplicate/unknown directive handling, and strict file/resource bounds |
| `CGRD-07` | LUT domain and sampling | freeze `DOMAIN_MIN/MAX` or equivalent mapping, dimension set, texel-center convention, trilinear versus tetrahedral interpolation, precision, out-of-domain behavior, and CPU reference indexing |
| `CGRD-08` | Grade composition | decide whether parametric and LUT transforms are both applied and in what order; define identity and contribution behavior without creating a blend stack |
| `CGRD-09` | State and identity | freeze global defaults, per-view override, requested/resolved/active/unavailable states, immutable generation digest, reload ordering, and completion-safe retirement |
| `CGRD-10` | Asset ownership | choose the source/cooked asset owner, canonical cooked representation, dependency/build/package membership, provenance fields, and whether generic TextureCooker can be extended without lying about semantics |
| `CGRD-11` | User and automation experience | freeze editor controls, asset diagnostics, reset/disable, live edit, capture labeling, command/manifest equivalence, and safe behavior when a LUT cannot activate |
| `CGRD-12` | Evidence and budgets | freeze fixtures, tolerances, backend comparison rule, artifact interpretation, LUT and transient memory ceilings, update latency, GPU budget, package matrix, and escalation triggers |

## Required Discovery Experiments

| Experiment | Question falsified | Required retained result |
| --- | --- | --- |
| `CGR-EXP-01` working-domain trace | can every input/output between reconstruction, exposure, grading, tone mapping, encoding, debug, UI, and capture be named without contradiction? | one resource/domain/format/extent diagram tied to current source and target owners |
| `CGR-EXP-02` SOP alternatives | do clamp, no-clamp, or signed-power choices diverge for negative/HDR values or break identity? | double-precision tables over frozen edge values and a selected rule |
| `CGR-EXP-03` LUT parser corpus | can the accepted `.cube` subset reject ambiguity, hostile sizes, non-finite data, wrong counts, and unsupported constructs? | grammar table, bounded corpus, expected disposition, parser/resource ceilings |
| `CGR-EXP-04` interpolation study | does trilinear meet the frozen analytic/known-cell error budget, or is tetrahedral required? | CPU comparison over identity, affine, discontinuous, and edge-domain LUTs |
| `CGR-EXP-05` asset route | can source identity, cook inputs, cooked bytes, runtime generation, package membership, reload, and retirement use existing owners? | owner/producer/consumer/lifetime/deletion ledger |
| `CGR-EXP-06` workflow walkthrough | can a developer select, understand, correct, disable, and automate a look without console-only knowledge? | reviewed first-use, invalid-asset, reload-race, package-missing, and reset journeys |
| `CGR-EXP-07` evidence dry run | will the selected checks detect swapped LUT axes, half-texel error, wrong luma coefficients, wrong order, silent identity fallback, and stale generation? | defect-seeded check design with fixed thresholds and artifacts |

## Risk Register

| ID | Cause, event, consequence | Prevention and detection | Contingency, owner, retirement |
| --- | --- | --- | --- |
| `RISK-CGR-01` | unnamed working primaries or exposure placement causes an attractive but non-portable look | `CGRD-02/03`; analytic domain trace and SDR/HDR comparison | block `DSP-5`; Renderer display owner retires after approved domain contract and defect-detecting evidence |
| `RISK-CGR-02` | `.cube` ambiguity or unbounded input creates wrong colors, memory pressure, or unsafe cooking | strict subset, checked counts, finite/domain validation, byte/dimension ceilings | reject transactionally and preserve prior generation; asset/cook owner retires after parser corpus passes |
| `RISK-CGR-03` | CPU parser, texture layout, and shader axis conventions disagree | one canonical layout and known-cell/asymmetric LUT fixtures | block activation; semantic owner retires after CPU/GPU indexed-cell proof |
| `RISK-CGR-04` | asynchronous edit/reload publishes stale or partial state | immutable request digest, generation-qualified publication, completion-safe retirement | retain last explicit good/default state with visible failure; View/runtime owner retires after randomized completion checks |
| `RISK-CGR-05` | a generic color-management framework grows around one small first-release feature | enforce `CGRD-01`, narrow public types, no OCIO runtime or multi-look graph | remove unused abstractions in the owning stage; Renderer owner retires at scoped diff review |
| `RISK-CGR-06` | raw, display-mapped, screenshot, or UI-composited artifacts are compared as if equivalent | named pre-grade/post-grade/pre-tone products and manifest fields | mark evidence inconclusive; acceptance owner retires after artifact-lineage checks |

## Discovery Acceptance

| ID | Pass criterion | Required evidence |
| --- | --- | --- |
| `AC-CGRD-01` | `CGRD-01` through `CGRD-12` have reviewed dispositions and no implementation-shaping `Unknown` remains. | signed decision table and invalidation triggers |
| `AC-CGRD-02` | every current and target stage has one color domain, format, extent, owner, and observable product. | `CGR-EXP-01` diagram and source ledger |
| `AC-CGRD-03` | parametric and LUT semantics are executable by independent CPU and shader implementations without inventing choices. | accepted `Semantics.md` revision plus `CGR-EXP-02` through `04` results |
| `AC-CGRD-04` | source/cooked/runtime identity and failure/retirement paths have one owner each and bounded resource policy. | `CGR-EXP-03/05` and deletion ledger |
| `AC-CGRD-05` | first-use, error, reload, package, capture, and automation experiences expose truthful requested/active state. | reviewed `UserExperience.md` and `CGR-EXP-06` |
| `AC-CGRD-06` | every `AC-CGR-*`, `FM-CGR-*`, and material risk maps to a predeclared defect-detecting check and budget. | no-orphan traceability plus `CGR-EXP-07` |

`FM-CGRD-01` occurs when a decision is silently deferred to code. `FM-CGRD-02` occurs when a selected oracle shares the implementation defect it claims to catch. `FM-CGRD-03` occurs when the package admits a control, asset form, domain, or workflow not mapped to acceptance and delivery. Any of these failures keeps the gate `Blocked`.

## Gate Decision

`CGRD-00` is **Blocked**. A reviewer may record `PASS` only when all discovery acceptance criteria pass conjunctively, the exact accepted revisions of the dossier, semantics, architecture, experience, and plan are named, and `DSP-5` plus release prerequisites permit work. A pass authorizes Stage 1 of [Plan](Plan.md), not the whole feature and not `FCR-REN-24`.

