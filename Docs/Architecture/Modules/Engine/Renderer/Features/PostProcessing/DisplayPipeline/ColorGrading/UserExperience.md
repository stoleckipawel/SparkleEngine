# Color Grading User Experience

**Status:** proposed development-product experience; blocked until `CGRD-00`, not an implemented UI promise

**Responsibility:** define discoverability, authoring, state truth, errors, live update, results, accessibility, support, and automation equivalence for first-release color grading

**Authority boundary:** [Execution Architecture](ExecutionArchitecture.md) owns runtime state/lifetime; [Semantics](Semantics.md) owns what controls mean; this page owns how a developer sees and operates those contracts

**Current readiness:** **0/100 — target only**.

## Product Priority Order

When concerns conflict, the first release chooses in this order:

1. truthful requested/resolved/active state and a recoverable viewport;
2. reproducible semantics and candidate-identifiable evidence;
3. neutral/default simplicity and fast first useful result;
4. responsive, coalesced live editing with bounded work;
5. richer artistic controls or presentation polish.

A visually smoother transition cannot hide that a requested LUT failed. More controls cannot precede a stable domain, neutral meaning, and capture identity.

## Product Pillar

**One explicit look, one truthful active generation, one reproducible route from editor intent to packaged pixels.** The user should not need to understand file parsing, upload fences, descriptor lifetime, or frame-graph internals, but the interface must expose enough semantic and identity detail to diagnose when the requested look is not the active look.

## Intended Person And Result

The first-release user is a developer or technical artist in DevelopmentEditor who needs to apply one reproducible global look to a viewport and verify that the same cooked look reaches the packaged candidate. The experience is not a professional color suite, shot timeline, volume editor, or runtime player-facing menu.

## Intended Jobs

| Person/job | Successful outcome | Explicitly unsupported shortcut |
| --- | --- | --- |
| rendering developer proving semantics | choose an asymmetric grade, capture raw pre/post products, and identify the exact semantic/runtime generation | eyeballing only the final encoded screenshot |
| technical artist authoring a look | adjust bounded parameters or select one admitted LUT and see pending/active truth | dropping arbitrary files into a packaged runtime |
| feature reviewer | reproduce neutral, known-cell, failure, and two-view cases from a manifest | relying on another machine's unstated editor session |
| build/package owner | see the logical LUT dependency and verify cooked-only runtime reachability | shipping source parser/editor dependencies |
| support investigator | copy one bounded status/error summary with candidate and asset lineage | mining an unbounded per-frame log |

## Frozen Defaults And Operational Budgets

Stage 0 must freeze each unresolved value before implementation:

| Item | Required freeze |
| --- | --- |
| default mode | `Off` or exact-neutral parameter state; one answer only |
| neutral values | slope `(1,1,1)`, offset `(0,0,0)`, power `(1,1,1)`, saturation `1`, no LUT unless semantics changes them explicitly |
| commit model | immediate, apply-button, or focus-loss semantics for numeric edits |
| live-edit debounce/coalescing | bounded delay and maximum pending work |
| convergence target | time from valid committed request to active generation under declared machine/profile |
| last-good retention | exact lifetime and whether it survives level/View/session changes |
| source/LUT ceilings | size/dimension/token limits presented to the user |
| error retention | when a failure clears and whether retry creates a new request generation |
| reset confirmation | whether reset is immediate and which values/asset/override it clears |
| capture destination/naming | deterministic, collision-safe, and product-identifying |

Unfrozen budgets make responsiveness and error behavior subjective and keep the UX blocked.

## First Use

1. Open the existing rendering/display settings surface.
2. Expand **Color Grading** and see the feature state: `Off`, `Parameters`, `Parameters + LUT`, `Pending`, or `Unavailable` with reason.
3. Adjust slope, offset, power, and saturation from neutral defaults, or choose one admitted `.cube` source asset.
4. The editor validates source/domain/dimension before committing a new request and shows requested versus active generation while cooking/uploading.
5. The viewport updates from one immutable generation; capture metadata names working space, parameter values, LUT content identity, and active generation.
6. **Reset to Neutral** removes the LUT request, restores exact neutral values, and makes the graph omission visible.

No console command, hidden file copy, restart, or manual shader recook is part of the normal accepted route unless discovery explicitly classifies it.

## View Session State And Dominant Action

| Visible state | Meaning | Dominant action | Actions that remain valid |
| --- | --- | --- | --- |
| `Off` | exact neutral request; no active LUT/pass | Enable or select mode | inspect defaults, import/select an asset |
| `Parameters` | accepted parameter-only generation is active | Edit or Reset | capture, compare, copy status |
| `Pending` | requested intent is valid but cook/load/upload has not converged | Cancel or wait | continue viewport use, inspect requested/active identities, reset |
| `Parameters + LUT` | requested and active digests match | Edit or Compare | capture, reset, select replacement |
| `Unavailable` | dependency/capability/capacity prevents activation | Retry or Deselect | inspect/copy error, retain explicit last-good/default state |
| `Rejected` | requested values/source violate the accepted contract | Correct input | inspect exact field/directive/limit; active state remains explicit |

The primary badge reflects the resolved state, not merely the enable checkbox. Pending and unavailable states show both “Requested” and “Currently visible.” If they differ, the interface names why and since which request generation.

## Primary Happy Path

1. Select the target viewport and open its existing display settings.
2. Confirm the working-space/stage label and whether values inherit the global default or override this View.
3. Choose **Parameters**, make a clearly asymmetric edit, and observe one committed request generation become active.
4. Optionally choose an already imported/cooked admitted `.cube` asset; inspect dimension, domain, source/cooked identity, and validation state before commit.
5. While pending, continue using the viewport and see the exact prior/default grade still visible.
6. When active, capture raw pre-grade, raw post-grade, and presentation products whose manifests share the same View/frame/grade lineage.
7. Use **Reset to Neutral** and verify that all controls, asset selection, override provenance, active state, and graph work return to the neutral contract.

The first release may offer a compact compare toggle only if Stage 0 decides its semantics and it cannot fork the active grade authority. A split-screen, before/after history, or look browser is not implied.

## Setup And Automatic Preflight

Before accepting a request, the experience checks the conditions it can know cheaply: parameter finiteness/range, asset type, parser/cook result, LUT dimension/domain/count, package reachability where applicable, backend shader availability, resource ceiling, and current View/output support. Preflight produces the same stable reason codes as runtime resolution.

Preflight is not proof of activation. It cannot claim success until the matching resident generation and rendered frame report the requested digest active. Conversely, transient pending work is not rendered as an error.

## Control Contract

| Control | Display requirement | Invalid-state behavior |
| --- | --- | --- |
| enable/mode | distinguish Off, Parameters, and Parameters + LUT | never show active when required LUT is unavailable |
| slope/offset/power | three labeled channels, neutral value, accepted range/domain help | identify offending channel/value and retain coherent active state |
| saturation | scalar with neutral `1`, domain/range help | reject or clamp exactly as semantics specifies |
| LUT asset | source name, dimension, domain, working-space metadata, content/generation identity | show parse/cook/load/upload reason; no silent substitute |
| requested/active | both identities and pending/fallback reason | remain visible until convergence or user cancellation |
| reset | one action with previewable neutral result | never retain hidden LUT or stale parameter override |

## Live Edit And Interruption

Rapid edits coalesce by request identity, not by mutating a live GPU resource. An older parse, cook, or upload completion cannot replace a newer request. Closing the viewport, loading another level, switching the selected look, or shutting down cancels or retires work through existing owners and leaves no partially published generation.

The viewport remains usable while a LUT request is pending. The last explicit good grade may remain active, but the UI must not imply that the requested look is visible. A user can cancel the pending request or reset to neutral without waiting for stale work.

Numeric edits that do not rebuild LUT content publish a new immutable parameter generation at the accepted commit boundary. LUT source changes flow through parse/cook/load/upload. Coalescing may skip superseded intermediate requests, but the UI retains the most recent committed request and never relabels a skipped generation active.

## Reset, Disable, Retry, Close, And Shutdown

| Action | Required behavior |
| --- | --- |
| Reset to Neutral | clear parameters, selected LUT, and View override as the accepted reset scope defines; invalidate eligibility of older completions; converge to exact pass omission |
| Disable | create an explicit neutral/off request; do not merely hide controls while work remains active |
| Cancel pending | cancel eligibility and bounded work where supported; a racing completion is discarded by generation identity |
| Retry | create a new request/work generation against the same logical intent and preserve the prior failure in evidence lineage |
| Change View/level | apply only the accepted default/override inheritance; never leak another View's request or active result |
| Close viewport | stop View-local publication, release pins after completion, and leave shared immutable resources to residency |
| Device recovery | show pending/unavailable while the eligible generation is re-established; never claim stale native resources active |
| Shutdown | bound/cancel CPU work, stop publication, drain through existing owners, and emit no modal prompt for invisible work |

## Capture And Artifact Experience

The capture action offers named products rather than ambiguous “screenshot”: pre-grade scene-linear, post-grade scene-linear, final presentation, and bounded status manifest. Raw products default to a lossless format that preserves negative/HDR values and carry the fields required by [Execution Architecture](ExecutionArchitecture.md#secondary-artifact-contract). Final screenshots are useful for review but are never substituted for raw numeric evidence.

On success, the UI shows the destination, artifact names, and candidate/check/grade identity. On partial failure it lists which products exist and which do not; it never labels an incomplete evidence bundle complete. Capture does not block ordinary viewport use longer than the Stage-0 budget.

## Errors And Recovery

| Condition | Message content | Safe state | Recovery action |
| --- | --- | --- | --- |
| unsupported `.cube` construct | file, directive/line, accepted subset | prior/default active state | edit/export supported source, then retry |
| wrong dimension/count/domain | expected and observed values | no new generation | correct asset and recook |
| missing/corrupt cooked asset | logical asset and package/load reason | request unavailable | recook/restage or deselect |
| upload/capacity failure | required bytes and current ceiling when available | prior/default active state | release resources or use no LUT, then retry |
| shader/pipeline missing | exact program/target identity | grade request unavailable | rebuild/cook owning target |
| backend divergence detected in evidence | candidate/backend/check identity | feature remains blocked | investigate owner; do not hide backend |

## Automation Equivalence

A noninteractive manifest may set the same parameter values, logical LUT asset, viewport/camera, backend, output extent, and capture products as the editor. It resolves the same semantic digest and produces a machine-readable requested/active/error result. Automation does not load arbitrary filesystem LUTs in a packaged runtime and does not bypass cook/package validation.

The machine-readable result uses the same closed state/reason vocabulary and includes the exact accepted/rejected inputs after canonical parsing. Exit status distinguishes semantic rejection, unavailable dependency/capability, execution failure, evidence mismatch, and success. Automation never waits forever for convergence; the timeout and cancellation result are explicit inputs/artifacts.

## Accessibility And Support

Controls use text and numeric values, not color swatches alone. Neutral/default values are visible; validation never depends only on red/green status color. Keyboard navigation and copyable asset/error identities are required. Support output is bounded to one request/active summary, semantic revision, LUT source/cooked/runtime identities, backend, stage/domain, and last failure; no per-frame log stream is added.

Channel fields have persistent labels and a deterministic tab order. Numeric input does not assume a decimal comma/point silently; display locale and manifest serialization are separate, with manifests using the frozen locale-independent grammar. Status icons include text. At supported UI scales, long asset identities and errors remain copyable without obscuring the primary action.

## Build And Reachability Matrix

| Surface | Controls/readout | Source parser | Cook | Runtime application | Evidence capture |
| --- | --- | --- | --- | --- | --- |
| DevelopmentEditor | yes | through owning import/cook workflow | yes | yes | full admitted set |
| noninteractive developer route | manifest/status only | no bypass; invokes admitted cook route when product permits | yes where explicitly requested | yes | full admitted set |
| packaged Runtime | no player-facing editor controls | absent | absent | cooked selected state only | bounded product scope only |
| release consumer path | no developer console/file picker introduced | absent | absent | only if product config admits selected grade | no developer-only surface |

Shipping or release reachability is not inferred from DevelopmentEditor success. Stage 0 and the release plan must explicitly decide which packaged products carry configurable grade state versus a cooked fixed selection.

## Stage-0 First-Use Dry Run

Before production UI work, a reviewer walks the proposed first-use, invalid source, pending replacement, reset, two-view, capture, and packaged-missing scenarios using mock state/results backed by the proposed vocabulary. The dry run records every hidden prerequisite, ambiguous label, impossible recovery action, missing identity, and state/action mismatch. It passes only if another reviewer can predict the active pixels and next legal action from the surface alone.

## Professional Defaults And Guardrails

- neutral is always one action away and is never a hidden preset;
- expert numeric entry is supported without requiring imprecise sliders;
- ranges/units/domains are visible and originate from the semantic contract;
- unsupported file constructs fail with actionable directive/line context, bounded to safe lengths;
- the interface never auto-resizes, approximates, or silently converts a LUT;
- changing output SDR/HDR profile does not relabel or mutate the scene-grade intent;
- compare/capture tools cannot create a second persistent grade state;
- destructive asset replacement or recook remains owned by the asset workflow, not a viewport widget.

## Common Experience Failure Points

- an enable checkbox says “on” while a missing LUT leaves neutral or prior-good pixels;
- inherited and per-view overrides look identical, causing edits to affect unexpected views;
- rapid edits activate out of order or errors from superseded requests replace current truth;
- reset restores visible numbers but leaves a hidden LUT/override/pass active;
- final screenshots omit working-space, stage, LUT, and active-generation identity;
- a packaged build exposes the control but lacks the cooked dependency or source-parser route;
- backend or device recovery leaves stale “active” state after native resources were invalidated;
- errors quote unbounded hostile source text or reveal machine-local paths unnecessarily;
- controls rely on color alone or truncate the only actionable field/limit;
- an automation route accepts inputs or fallback behavior that the editor rejects.

## Experience Exit

The experience contract is ready only when first use, neutral reset, invalid source, rapid edit, cancellation, two-view isolation, package-missing, capture interpretation, and manifest equivalence have binary checks with predeclared results. A screenshot of the settings panel or a visually pleasing look is insufficient.

## UX Review And Acceptance Handoff

The UX review hands the acceptance owner: the frozen state/action/reason table; default and budget record; first-use dry-run notes; editor and manifest field mapping; error fixture catalog; accessibility/input/scale checklist; capture artifact examples; build/reachability matrix; and checks that detect hidden fallback, stale completion, reset residue, View leakage, and editor/automation divergence.

The reviewer records `PASS` only when each observable state derives from the same runtime owners defined by [Execution Architecture](ExecutionArchitecture.md), every control meaning matches [Semantics](Semantics.md), and excluded surfaces remain unreachable. Mockups, documentation, or editor-only interaction do not prove the packaged route.

## Sources And Precedent

View association, immutable resource generations, activation validation, and separated authored/generated/applied identities are informed by the pinned Filament, Unity, OpenColorIO, and security references in [Research](Research.md#source-ledger). They are workflow and failure precedent only. This page does not adopt an external UI, volume system, file browser, default range, or proof artifact.
