# Color Grading User Experience

**Status:** proposed development-product experience; blocked until `CGRD-00`, not an implemented UI promise

**Responsibility:** define discoverability, authoring, state truth, errors, live update, results, accessibility, support, and automation equivalence for first-release color grading

**Authority boundary:** [Execution Architecture](ExecutionArchitecture.md) owns runtime state/lifetime; [Semantics](Semantics.md) owns what controls mean; this page owns how a developer sees and operates those contracts

**Current readiness:** **0/100 — target only**.

## Intended Person And Result

The first-release user is a developer or technical artist in DevelopmentEditor who needs to apply one reproducible global look to a viewport and verify that the same cooked look reaches the packaged candidate. The experience is not a professional color suite, shot timeline, volume editor, or runtime player-facing menu.

## First Use

1. Open the existing rendering/display settings surface.
2. Expand **Color Grading** and see the feature state: `Off`, `Parameters`, `Parameters + LUT`, `Pending`, or `Unavailable` with reason.
3. Adjust slope, offset, power, and saturation from neutral defaults, or choose one admitted `.cube` source asset.
4. The editor validates source/domain/dimension before committing a new request and shows requested versus active generation while cooking/uploading.
5. The viewport updates from one immutable generation; capture metadata names working space, parameter values, LUT content identity, and active generation.
6. **Reset to Neutral** removes the LUT request, restores exact neutral values, and makes the graph omission visible.

No console command, hidden file copy, restart, or manual shader recook is part of the normal accepted route unless discovery explicitly classifies it.

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

## Accessibility And Support

Controls use text and numeric values, not color swatches alone. Neutral/default values are visible; validation never depends only on red/green status color. Keyboard navigation and copyable asset/error identities are required. Support output is bounded to one request/active summary, semantic revision, LUT source/cooked/runtime identities, backend, stage/domain, and last failure; no per-frame log stream is added.

## Experience Exit

The experience contract is ready only when first use, neutral reset, invalid source, rapid edit, cancellation, two-view isolation, package-missing, capture interpretation, and manifest equivalence have binary checks with predeclared results. A screenshot of the settings panel or a visually pleasing look is insufficient.

