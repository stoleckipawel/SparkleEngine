# Chromatic Aberration User Experience

**Status:** proposed development-product experience; blocked until `CHRD-00`, not an implemented UI promise

**Responsibility:** define discoverability, controls, active-state truth, first use, reset, error/recovery, debug/UI/capture interpretation, accessibility, support, and automation equivalence for the admitted lens effect

**Authority boundary:** [Semantics](Semantics.md) owns the meaning of every control and pixel; [Execution Architecture](ExecutionArchitecture.md) owns runtime state, graph, and lifetime; [Discovery](Discovery.md) freezes unresolved product choices; this page owns observable workflow only

**Current readiness:** **0/100 — target only**. No selector, state, pass, control, or workflow exists in committed revision `ca55e7d8`.

## Product Priority Order

1. Exact `Off` versus active truth and preservation of the normal display path.
2. Stable authored units and reproducible intentional-fringe identity.
3. Clear first use, neutral reset, and actionable invalid/unavailable recovery.
4. Responsive per-view live editing with no cross-view or stale-frame state.
5. Artistic convenience and polish.

The interface must never make incidental renderer fringes appear intentional simply because a checkbox is enabled. A smoother widget or more dramatic default cannot outrank semantic stability or truthful active state.

## Product Pillar

**A small intentional lens accent that is easy to turn on, exact to turn off, and impossible to confuse with an unrelated image defect.**

## Intended People And Jobs

| Person/job | Desired result | Not part of the first release |
| --- | --- | --- |
| technical artist | set a restrained global/per-view fringe with stable units and reset it exactly | physical lens matching, spectral/profile authoring, local volume blending |
| rendering developer | exercise analytic patterns, inspect stage/product identity, and diagnose channel/edge/domain defects | tuning by final screenshot alone |
| feature reviewer | reproduce one manifest across resolutions, views, backends, and package cells | private console knowledge or manually edited runtime files |
| support investigator | distinguish active effect output from reconstruction/filtering artifacts using a bounded snapshot | unbounded frame logs or hidden internal handles |

## Frozen Defaults And Operational Budgets

Stage 0 records one value for every row:

| Item | Required decision |
| --- | --- |
| default availability/state | default `Off`; whether the section is always visible or capability-gated |
| default values | strength, center, start offset, inheritance/override state |
| units/labels | exact reference-height pixels, normalized center, start-radius meaning, model name if exposed |
| admitted ranges | numeric min/max/step/text precision and reject-versus-clamp behavior |
| commit model | immediate or explicit apply; keyboard and text entry behavior |
| convergence budget | committed edit to matching active frame under declared profile |
| error retention | when rejected/unavailable clears and how retry occurs |
| reset scope | values, View override, and active state cleared by one action |
| capture products | defaults, location/naming, maximum wait/bytes, partial-failure result |
| support summary | fields, bounded size, copy/export action, sensitive-path treatment |

No implementation stage may invent a range, default, status, or timeout because the UI library needs one.

## Placement And One-Click Entry

Chromatic Aberration lives in the existing rendering/display settings surface, adjacent to the owning display-pipeline stage—not in camera transform settings, texture import, a generic effects marketplace, or an RHI/backend panel. The entry shows `Off`, `Active`, `Rejected`, or `Unavailable` without expanding it.

Enabling from exact defaults either remains `Off` until a nonzero strength is committed or applies a small, explicitly frozen nonzero default; Stage 0 must choose. The surface names the effect as intentional post-processing and links/copy-exposes its target-linear/pre-encode/pre-UI stage so users do not interpret it as a fix for color fringing defects.

## First Use And Primary Happy Path

1. Select one viewport and open **Display > Chromatic Aberration**.
2. Confirm whether values inherit the global default or override this View, and confirm the current state is `Off`.
3. Set a nonzero strength using its explicit `px @ 1080 lines` label; optionally adjust normalized center and start offset.
4. The request validates immediately or at the accepted commit boundary and the next eligible frame reports the exact settings digest active.
5. Inspect an intentional-fringe status/capture marker that names View, frame, domain, extent, semantic model, and active values.
6. Change output resolution/aspect or another viewport and observe stable unit behavior and isolation.
7. Choose **Reset to Off** and observe neutral values, removed View override per the frozen reset scope, and no effect pass/resource.

No shader recook, console command, restart, manual file copy, or backend-specific control is part of the normal route.

## Control Contract

| Control/readout | Meaning shown to user | Invalid/unavailable behavior |
| --- | --- | --- |
| state/enable | requested and active `Off`/`Active`/error, not just a checkbox | mismatch is explicit; no silent disable |
| strength | maximum displacement in frozen reference-height units and sign policy | offending value/range named; authored/effective values both shown if sanitization is admitted |
| center X/Y | active-viewport normalized center, independent of window/DPI | exact field and admitted range named |
| start offset | normalized radius where falloff begins, with frozen center/radius convention | unstable boundary rejected before active state |
| inheritance/override | global default versus this View and reset destination | never edits another View implicitly |
| domain/model | read-only semantic revision/model and current SDR/HDR target-linear domain | unsupported profile reports unavailable rather than changing math |
| requested/active | canonical values/digests and reason when different | remains visible through recovery/reset |
| capture | named pre-lens/post-lens/final products and manifest | partial output listed; no false complete result |
| reset | previewable exact neutral/off scope | invalidates prior non-neutral eligibility and hidden overrides |

Sliders may provide coarse artistic adjustment, but exact numeric entry is authoritative. Controls never expose spectral LUT, wavelength constants, sampler/filter, quality tier, history, guard band, or backend implementation.

## View Session State And Dominant Action

| State | Pixels/work | Dominant action | Other valid actions |
| --- | --- | --- | --- |
| `Off` | normal display route; no effect pass/resource | enter nonzero strength | inspect defaults, choose override, capture baseline |
| `Active` | matching nonzero request scheduled for this View/frame | edit or Reset | capture, copy status, compare with baseline |
| `Rejected` | request violates semantic/range/finite contract; effect omitted | correct offending field | reset, copy error, retain authored text if safe |
| `Unavailable` | values valid but domain/program/device/product cannot execute; effect omitted | retry after stated condition or Reset | copy reason, use viewport normally |
| optional `Pending` | only if program materialization is truly asynchronous; current normal route remains | cancel/wait | reset, copy requested/current state |

If Stage 0 does not admit `Pending`, it must not appear in the runtime or UI vocabulary. Every state maps to exactly one runtime result and one machine-readable result.

## Live Editing And Frame Coherence

Edits create immutable request generations. Coalescing may skip intermediate requests but may not activate them later or report them as rendered. Each frame uses one complete settings/domain/extent/program generation; a resize, profile switch, or edit becomes a later frame.

The viewport remains responsive because this effect has no source asset or background build. If program/pipeline materialization is not ready, the state is explicitly unavailable/pending and the normal display route remains active. Editing one View changes only that View unless the user is visibly editing the global default.

## Reset, Disable, Retry, Close, And Shutdown

| Action | Required behavior |
| --- | --- |
| Reset to Off | restore frozen neutral values, clear the selected override scope, publish neutral generation, and omit pass/resource |
| disable | identical semantic result to exact neutral; no hidden active strength |
| retry | re-evaluate the same valid request against a new dependency/device/frame generation; never mutates an old frame |
| resize/minimize | admit only coherent positive extents; no divide/dispatch at zero; resume with recomputed derived values |
| switch SDR/HDR/debug mode | resolve the new domain/classification and show unavailable if not admitted; never sample encoded/exact-debug/UI products accidentally |
| close viewport/level | stop View-local publication and release only ordinary frame resources after completion |
| device recovery | show unavailable until eligible program/native state exists; do not label stale state active |
| shutdown | no effect-owned worker/history needs draining; ordinary frame/program owners complete normally |

## Errors And Recovery

| Condition | Message contract | Safe pixels/state | Recovery |
| --- | --- | --- | --- |
| non-finite/out-of-range field | field, authored value, accepted range, reject/sanitize rule | effect omitted or exact documented effective value; never hidden default | correct value or reset |
| zero/minimized/incoherent extent | View/frame identity and extent reason | no effect dispatch; normal minimized behavior | restore valid extent |
| unsupported SDR/HDR/debug/product cell | requested cell and admitted matrix | effect unavailable; normal display route | select admitted cell or wait for feature scope |
| missing shader/program/pipeline/binding | exact program/backend/build/device identity | effect unavailable; no mislabeled fringe | rebuild/cook/recover owning dependency |
| graph/input failure | product/generation/edge reason | renderer's normal explicit frame failure; no stale lens output | repair owning graph/product route |
| backend divergence/evidence failure | candidate/backend/check/artifact IDs | acceptance remains blocked, not silently disabled | investigate and rerun frozen check |

Messages are bounded and copyable. They do not expose unsafe machine-local paths or depend on color alone.

## Debug, UI, Comparison, And Artifact Meaning

Exact debug views bypass the effect. Display-intent views follow the classification owned by Debug Views and show whether aberration is active. UI remains downstream and therefore sharp; an alpha/UI sentinel is part of acceptance. If an overlay labels effect state, it must be excluded from raw semantic products.

An `Off`/`Active` comparison uses the same source frame/camera/extent/domain where feasible and carries both digests. It is a review aid, not a second persistent state or temporal history. Any camera motion/frame mismatch is visible in the manifest.

Intentional effect captures are named `PreChromaticAberration`, `PostChromaticAberration`, and final presentation (final names follow accepted graph vocabulary). They include active state/model/settings, product domain/format, View/frame/extent/subrect, backend/device/program generation, candidate, and check ID. A colored edge without that lineage remains an artifact, not feature proof.

## Automation And Runtime Equivalence

A developer manifest can select the same global/per-view provenance and exact numeric values, viewport/camera, output extent/subrect, backend/profile, frames, and capture products as the editor. It receives the same canonical digest, closed state/reason code, active frame identity, artifacts, and timeout result. Locale-independent manifest values do not inherit editor display locale.

Packaged Runtime consumes only values admitted by its product configuration. It has no developer settings panel, console-only enable path, spectral/profile asset, or arbitrary file input. A DevelopmentEditor workflow does not establish release-consumer reachability.

## Accessibility, Input, Locale, And Scale

Every control has persistent text, unit, neutral/default, range, and state labels. Numeric text entry and keyboard navigation are supported; sliders are not the only route. Status uses text plus icon, never color alone. Tab order follows enable/state, strength, center, start, inheritance, reset, diagnostics/capture.

Editor display respects locale, while persisted/manifests use the frozen locale-independent representation. At supported UI scale, long reason/program/candidate identities wrap or copy without hiding the dominant action. Focus, validation, and reset are usable without precise pointer dragging.

## Build And Reachability Matrix

| Surface | Authoring | Application | Diagnostics/capture |
| --- | --- | --- | --- |
| DevelopmentEditor | full admitted controls and per-view override | yes on admitted backend/domain | full bounded workflow |
| noninteractive developer route | manifest fields only | same prepared/runtime path | machine-readable result and admitted products |
| packaged Runtime | fixed/cooked configuration only if product scope admits it | same shader semantics | bounded product-supported result only |
| release consumer UI | no new player-facing control | product-configured behavior only | no developer panel/console promise |

## Stage-0 First-Use Dry Run

An independent reviewer walks `Off -> Active -> edit -> resolution/aspect change -> two Views -> invalid -> unavailable -> retry -> capture -> reset`, plus SDR/HDR/debug/UI/package cells, using the proposed state/action tables and analytic pattern mockups. The review records hidden prerequisites, ambiguous units, stale-state possibilities, and places where incidental fringing could be misclassified. It passes only when the reviewer can predict whether the pass executes, which pixels are intentional, and the next legal recovery action without private explanation.

## Professional Defaults And Guardrails

- default `Off` protects image-quality diagnosis and costs nothing;
- no dramatic nonzero value activates merely by opening/expanding the section;
- maximum strength is bounded by the accepted quality, edge, and performance study;
- no automatic spectral mode, guard band, or quality switch appears by backend/resolution;
- changing output profile preserves authored intent but re-resolves domain support truthfully;
- reset cannot leave an invisible override, pass, or stale status;
- ordinary reconstruction/filtering fringes continue to use defect vocabulary;
- capture/status labels use the intentional effect name only when active identity matches.

## Common Experience Failure Points

- enable says on while neutral, invalid, or missing-program state schedules no effect;
- strength lacks units and changes appearance materially with resolution;
- center is interpreted in window/backing coordinates, especially for subrect or DPI cases;
- per-view override edits the global default or leaks to another viewport;
- reset zeros strength but leaves an override/model/status/pass active;
- UI or exact debug overlays are distorted and mistaken for intended behavior;
- final screenshots omit the `Off` baseline, stage, active values, or source frame;
- packaged state exposes controls without an admitted configuration/program route;
- automation clamps/rejects differently from the editor;
- error and success are distinguished only by colored fringe appearance.

## UX Review And Acceptance Handoff

The UX handoff includes frozen defaults/ranges/units; control-to-semantic mapping; state/action/reason vocabulary; first-use transcript; reset/two-view/resize/minimize/profile/debug/UI/error/recovery cases; editor-to-manifest field map; accessibility/input/locale/scale review; capture examples; reachability matrix; and defect controls for silent disable, stale frame, cross-view leakage, reset residue, and artifact misclassification.

The experience passes only when visible truth derives from the prepared/runtime owners, exact neutral is obvious and cheap, all errors preserve a usable display route, automation matches editor semantics, and excluded surfaces remain absent. A polished panel or attractive fringe image is not acceptance.

## Sources And Precedent

The compact controls and model alternatives are informed by the revision-pinned AMD, Unity, and Unreal primary sources in [Research](Research.md#source-ledger). They are workflow/product precedent only. No external UI, default, range, screenshot, spectral asset, or model is adopted by this page without `CHRD-00` disposition.
