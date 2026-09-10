# Color Grading Discovery

**Status:** acceptance contract; `CGRD-00` is `Blocked`, and production implementation is not authorized

**Responsibility:** freeze the first-release color domain, transform semantics, LUT contract, ownership, experience, budgets, and evidence protocol before `DSP-5` implementation begins

**Authority boundary:** [Research](Research.md) supplies precedent; [Semantics](Semantics.md), [Execution Architecture](ExecutionArchitecture.md), and [User Experience](UserExperience.md) are conditional design candidates; [Plan](Plan.md) orders work only after this gate passes; [README](README.md) owns feature acceptance; code and build configuration own implementation

**Verified:** 2026-09-10 against committed revision `30597d7d`; current Renderer, shader, texture-cooking, settings, editor, and package routes were re-inspected as source only; unrelated dirty work was present outside this package

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

`PASS` freezes one immutable `CGRD-00-R1` report and permits only Stage 1 of [Plan](Plan.md). It does not mean that a parser is safe, a shader compiles, a LUT is colorimetrically correct, a backend agrees, a package contains the asset, or `FCR-REN-24` passed. A decision that can change feature scope, color meaning, source grammar, identity, ownership, failure, cost, or evidence keeps the gate blocked.

## Iteration Control Record

| Field | `ITER-CGRD-00` |
| --- | --- |
| Claim | Sparkle has enough reviewed, falsifiable information to implement the smallest color-grading result without inventing color, asset, lifecycle, or evidence policy in code. |
| Starting point | committed source and package input `30597d7d`; unrelated dirty work outside this feature remains user-owned |
| North Stars | `NS-MATH-DATA`, `NS-OWNERSHIP`, `NS-EVIDENCE`, `NS-SIMPLIFY`, `NS-ADOPTION` |
| Persona targets | `PGE-02`, `PGE-05`, `PGE-07`, `PGE-08`, `PGE-09`, `PGE-13`, `PGE-15` |
| Delivery targets | `CGRD-00`, `DSP-5`, preparation for `FCR-REN-24` |
| Technical risks | `RISK-CGR-01` through `RISK-CGR-09`; source-file parsing is untrusted-input work, not a cosmetic tool concern |
| Decision | **BLOCKED** until `AC-CGRD-01` through `10` pass at one immutable report revision and the repository owner records acceptance |

## Discovery Scope

Included:

- one exact scene-working boundary and one bounded parametric-plus-LUT product;
- current owner, selector, source/cook/runtime asset, frame-graph, shader, View, capture, package, backend, lifetime, and failure traces;
- a strict `.cube` subset with byte, line, token, dimension, sample-count, allocation, numeric, and path/provenance limits;
- exact CPU/GPU semantic correspondence, asymmetric fixtures, raw-stage artifacts, and a defect-injection oracle ladder;
- DevelopmentEditor authoring and packaged Runtime consumption with one serialized intent and truthful requested/resolved/active state;
- a dependency-ordered clean-break plan with estimates, stop rules, deletion targets, reviewers, and candidate handoff.

Excluded:

- production code, submitted test-only infrastructure, copied external LUTs, or candidate images during discovery;
- OpenColorIO runtime integration, a general color-management graph, local volumes, multiple blended looks, curves, masks, timelines, or display calibration;
- declaring the current tone mapper, output encoding, generic texture cooker, or any future screenshot a grading implementation or correctness proof.

## Decision Recording Contract

Every `CGRD-*` row must retain: `Proposed`, `Accepted`, `Rejected`, or `Blocked`; accountable owner and independent reviewer; exact evidence/artifact; rationale and rejected alternatives; affected `CGR-FS-*`, `CGR-MATH-*`, `AC/FM/CHK`; implementation and package consequence; and an invalidation trigger. A default value, source precedent, prototype result, or convenient API shape is not a disposition.

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
| `RISK-CGR-07` | a hostile `.cube` token, line, count, or path exhausts memory/CPU or escapes the accepted source root | bounded streaming/token parser, checked arithmetic before allocation, canonicalized allowed roots, no format-string parsing, corpus/fuzz review | reject before publication, preserve prior good generation, and record one stable category; asset/cook and security reviewers retire after all boundary/fault cases settle within budget |
| `RISK-CGR-08` | editing global defaults or one viewport resets unrelated temporal histories or another view's grade | generation impact taxonomy and per-view immutable selection; digest-difference and two-view interleaving checks | discard only the affected grade generation and repair the producer; View owner retires after reset-locality evidence |
| `RISK-CGR-09` | parameter/LUT fusion, precision reduction, or texture filtering changes semantics after standalone checks pass | freeze independent products and semantic digest before optimization; compare fused/unfused CPU/GPU outputs and counters | retain the unfused route or reject the optimization; pass owner retires after equivalence and performance evidence at accepted thresholds |

## Discovery Acceptance

| ID | Pass criterion | Required evidence |
| --- | --- | --- |
| `AC-CGRD-01` | `CGRD-01` through `CGRD-12` have reviewed dispositions and no implementation-shaping `Unknown` remains. | signed decision table and invalidation triggers |
| `AC-CGRD-02` | every current and target stage has one color domain, format, extent, owner, and observable product. | `CGR-EXP-01` diagram and source ledger |
| `AC-CGRD-03` | parametric and LUT semantics are executable by independent CPU and shader implementations without inventing choices. | accepted `Semantics.md` revision plus `CGR-EXP-02` through `04` results |
| `AC-CGRD-04` | source/cooked/runtime identity and failure/retirement paths have one owner each and bounded resource policy. | `CGR-EXP-03/05` and deletion ledger |
| `AC-CGRD-05` | first-use, error, reload, package, capture, and automation experiences expose truthful requested/active state. | reviewed `UserExperience.md` and `CGR-EXP-06` |
| `AC-CGRD-06` | every `AC-CGR-*`, `FM-CGR-*`, and material risk maps to a predeclared defect-detecting check and budget. | no-orphan traceability plus `CGR-EXP-07` |
| `AC-CGRD-07` | the full included/excluded feature, profile, backend, asset, selector, and output matrix has one disposition and no reachable undeclared cell. | reconciled `CGR-FS-*` ledger, support matrix, selector/package audit, and explicit exclusions |
| `AC-CGRD-08` | hostile-input and capacity policy is sufficient to bound parser work, multiplication/allocation, path resolution, decoded payload, GPU upload, retirement backlog, and diagnostic volume. | adversarial corpus, checked-limit table, timeout/memory bounds, cleanup observation, and security review |
| `AC-CGRD-09` | each stage is a reviewable vertical result with prerequisites, estimates, deletions, non-goals, stop conditions, and a prompt whose non-negotiables are falsifiable. | accepted `Plan.md`, capacity/estimate assumptions, and stage-to-acceptance map |
| `AC-CGRD-10` | an independent reviewer can reconstruct the claim, reproduce at least one hand case, and identify a seeded defect caught by each evidence class. | exact-revision color/math, asset/security, architecture, UX, and evidence review record |

`FM-CGRD-01` occurs when a decision is silently deferred to code. `FM-CGRD-02` occurs when a selected oracle shares the implementation defect it claims to catch. `FM-CGRD-03` occurs when the package admits a control, asset form, domain, or workflow not mapped to acceptance and delivery. Any of these failures keeps the gate `Blocked`.

## Discovery Failure Modes

| ID | Controlled failure | Required safe result | Detecting checks |
| --- | --- | --- | --- |
| `FM-CGRD-01` | an implementation prompt, type name, or shader constant selects an unresolved `CGRD-*` choice | Stage 1 remains unauthorized; move the choice back to Discovery and invalidate dependent design text | `CHK-CGRD-01`, `CHK-CGRD-08` |
| `FM-CGRD-02` | CPU reference and GPU route share parser, layout, luma weights, or transform helper | evidence is invalid rather than agreeing; install an independent evaluator and asymmetric fixtures | `CHK-CGRD-03`, `CHK-CGRD-04` |
| `FM-CGRD-03` | included control, asset form, backend/profile, state, or output lacks an owner/stage/criterion/failure/check | no-orphan gate fails and the surface remains excluded/unreachable | `CHK-CGRD-02`, `CHK-CGRD-09` |
| `FM-CGRD-04` | parser limits are described qualitatively or allocation occurs before all counts are validated | discovery remains blocked; retain hostile input and memory/time observations | `CHK-CGRD-05` |
| `FM-CGRD-05` | a visual comparison, vendor LUT, or screenshot is the only transform oracle | verdict is `Inconclusive`; add analytic/hand-known values and raw-stage artifacts | `CHK-CGRD-03`, `CHK-CGRD-06` |
| `FM-CGRD-06` | package or live-reload behavior is postponed until after semantic implementation | plan review fails because identity/failure could change architecture | `CHK-CGRD-07`, `CHK-CGRD-08` |

## Check Design Ledger

Every retained check must declare initial state, action or injected fault, independent oracle, matrix, threshold, artifact, maximum work/resource bound, cleanup, and escalation. The Stage-0 report fills exact commands and locations; this table defines the minimum claims they must falsify.

| ID | Smallest falsifier | Oracle and required artifact | Fails when |
| --- | --- | --- | --- |
| `CHK-CGRD-01` | scan the package and prompts for unresolved decision leakage | `CGRD-*` disposition export plus plan/prompt reference map | any implementation-shaping slot is absent, contradictory, or silently chosen |
| `CHK-CGRD-02` | enumerate feature statements, profiles, selectors, asset forms, products, and exclusions | no-orphan cross-document traceability table | a reachable/included surface lacks one owner or proof route |
| `CHK-CGRD-03` | evaluate neutral, asymmetric SOP/saturation, negative, HDR, and order hand cases | independent double-precision calculations retained as text/CSV | a semantic alternative is indistinguishable or a known mutation survives |
| `CHK-CGRD-04` | evaluate asymmetric `2^3`/`3^3` LUT cells and interior interpolation | independently generated LUT bytes, expected coordinates, and CPU results | axis order, half-texel, interpolation, domain, or composition defects survive |
| `CHK-CGRD-05` | feed truncated, extra, duplicate, non-finite, huge-count, huge-token, hostile-path, and advisory-shaped inputs | bounded parser/resource/security report with peak bytes/time and stable categories | work/allocation is unbounded, partial state publishes, or a crash/hang occurs |
| `CHK-CGRD-06` | trace pre-grade, post-parameter, post-LUT, pre-tone, encoded, and screenshot products | domain/format/extent/alpha manifest and seeded stage-order defects | an artifact is ambiguous or a wrong-stage transform can pass visually |
| `CHK-CGRD-07` | model source edit/cook/upload/publication/replacement/retirement interleavings | generation state table with stale-completion injections | stale/partial data activates, prior good state corrupts, or resources leak |
| `CHK-CGRD-08` | dry-run every plan stage from its prompt | reviewer transcript with prerequisite and stop-condition challenges | the executor must invent policy, begin a later stage, or cannot name deletion/exit proof |
| `CHK-CGRD-09` | map every risk/failure to a controlled negative and criterion | machine-readable or reviewed mapping with zero orphan IDs | any risk has no prevention/detection/contingency/retirement evidence |
| `CHK-CGRD-10` | independent color, asset/security, architecture, UX, and evidence review | named corrections and final exact-revision dispositions | reviewers require private explanation or find a claim-changing ambiguity |

## Required `CGRD-00-R1` Evidence Package

The gate report is complete only when it retains:

1. exact source revision, dirty-state boundary, current-route trace, and negative-capability evidence;
2. all `CGRD-01` through `12` dispositions with rationale, alternatives, reviewers, and invalidation triggers;
3. accepted feature/support matrices, one terminology/domain glossary, and accepted revisions of every companion document;
4. working-space/exposure/order derivation, SOP/saturation/LUT hand cases, axis/interpolation fixtures, and defect controls;
5. `.cube` grammar plus hostile-input corpus, security review, checked capacity limits, expected categories, and package/provenance policy;
6. owner/producer/consumer/identity/lifetime/publication/retirement/deletion ledger and end-to-end author-to-pixel sequence;
7. first-use, invalid asset, live edit, two-view, reset, capture, package, accessibility, and automation dry runs;
8. stage estimates and assumptions, no-orphan traceability, exact stop/escalation rules, and independent review record;
9. exact checks actually run, artifacts, limitations, unavailable executable checks, and final `PASS` or `BLOCKED` decision.

## Gate Decision

`CGRD-00` is **Blocked**. A reviewer may record `PASS` only when all discovery acceptance criteria pass conjunctively, the exact accepted revisions of the dossier, semantics, architecture, experience, and plan are named, and `DSP-5` plus release prerequisites permit work. A pass authorizes Stage 1 of [Plan](Plan.md), not the whole feature and not `FCR-REN-24`.
