# Basic Vignette

**Status:** target feature dossier; release requirement authorized, implementation and numerical acceptance unclaimed

**Scope:** an optional, bounded scene-image darkening toward the viewport corners, separate from chromatic aberration

**Verified:** source/name search at `93e86b8113c3fcd0c4921729c3513538b95bc794` on 2026-10-10 found no vignette feature

**Current readiness:** **0/100 — target only**; included in the lens post-processing scope of [FCR-REN-25](../../../../../../../Acceptance/FeatureCompletionReports.md#initial-completion-report-registry), without changing the existing portfolio score

**Parent:** [Display Pipeline](README.md). [Release requirements](../../../../../../../Acceptance/FirstRelease.md#required-rendering-closure) require this small effect on both backends. This page owns its proposed semantics and local delivery checks; release results belong in the existing FCR report.

## Result And Bounds

The user can enable a basic vignette, adjust its intensity and inner radius, restore neutral defaults and observe the same result in D3D12 and Vulkan. It is an artistic post-process example, not a physical lens model. No texture asset, lens profile, animated authoring system, barrel distortion, film grain or new general effect registry is required.

| Choice | Target contract |
| --- | --- |
| Placement | Target-linear scene image after the selected SDR/HDR tone/gamut mapping, before chromatic aberration, output encoding and UI. Raw reference radiance and exact diagnostic products bypass both lens effects. |
| Controls | Intensity in `[0,1]`, default `0`; inner radius in `[0,0.99]`, default `0.5`. Reject non-finite/out-of-range input at the owning settings boundary; retain last valid value and expose an actionable error. Names and persistence membership are frozen before implementation. |
| Coordinates | For local viewport UV and nonzero output extent, `r = length((uv - 0.5) * extent) / (0.5 * length(extent))`. Radius is zero at the center and one at the corners, with aspect-correct pixel distances. Use the viewport's own extent/subrect, not the containing host window. |
| Mask | `w = smoothstep(innerRadius, 1, r)`; `RGB_out = RGB_in * (1 - intensity * w)`. Copy alpha unchanged. No sampling beyond the input pixel and no new history. |
| Neutral state | Intensity zero schedules no effect pass/resource/copy; its output is the same input product. The image must be bit-identical to the disabled route. |
| Composition | Multiplication applies once in the declared linear domain. No metering, DLSS guide, raw reference, HUD or UI darkening. Chromatic aberration retains its own coordinates and alpha policy. |
| Ownership | Renderer-private display feature owns pixel policy/parameters and typed frame-graph pass. Existing settings authority owns values; Application owns persistence; Editor/runtime settings presenters own controls only. No native API or feature policy enters generic graph/host code. |

The formula and placement are proposed design choices, not SDK facts. Before code, freeze formats, dispatch coverage, floating-point tolerances, supported extent/aspect range, HDR treatment and pass-cost budget in this owner using the existing display-domain contract. Do not choose thresholds after seeing candidate results.

## Acceptance And Failure Checks

| Criterion | Required check and retained evidence |
| --- | --- |
| `AC-VIG-01` | Intensity zero yields exact input bytes and no vignette pass/allocation in the native capture on both APIs. |
| `AC-VIG-02` | Independent CPU formula and GPU output agree within the predeclared format tolerance for center, edges/corners, gradients and uniform inputs; both APIs agree under that same tolerance. |
| `AC-VIG-03` | Resize, non-square extent, viewport subrect and two distinct views use their own extents/settings; no stale shared state or host-window mask. |
| `AC-VIG-04` | Alpha, raw reference, exact debug products and UI remain unchanged; scene vignette occurs once before output encoding on SDR and qualified HDR tuples. |
| `AC-VIG-05` | Invalid values are visibly rejected; reset returns neutral defaults; state persists only in the declared per-user root. Included Shipping behavior has no editor/tool dependency. |
| `AC-VIG-06` | Record pass CPU/GPU cost, resource/copy delta and complete-frame effect against the fixed budget; no permanent history or hidden work while disabled. |

`CHK-VIG-01` exercises the neutral/corner/ramp formula and alpha controls (`AC-VIG-01/02/04`); `CHK-VIG-02` exercises extent/view/settings transitions (`AC-VIG-03/05`); `CHK-VIG-03` captures order, package membership and bounded cost (`AC-VIG-04/05/06`). All run on the same candidate and both APIs after the discovery card freezes numeric tolerances and budgets. A documentation check cannot pass them.

| Failure | Detection and safe result |
| --- | --- |
| `FM-VIG-01`: NaN/Inf/out-of-range controls or zero extent | Settings/viewport boundary rejects before recording; preserve valid settings, report the failing value/extent; no NaN output or partial pass. `CHK-VIG-02` controlled input. |
| `FM-VIG-02`: wrong domain/order or UI/raw product affected | Native markers and raw/formula comparison expose the defect; fail `AC-VIG-04` rather than tune intensity to conceal it. `CHK-VIG-01/03` controlled product/order cases. |
| `FM-VIG-03`: stale extent/view value after resize | Generation-qualified viewport transition rejects stale work; no cross-view state; fail `AC-VIG-03` if the old mask appears. `CHK-VIG-02` transition cases. |

## Remaining Delivery

1. Reconcile existing settings, target-linear stage and graph product ownership; freeze the numeric/check card and justified integration-hook ledger. Preserve the separate chromatic-aberration discovery.
2. Add only this feature's parameters/pass and existing settings/persistence/presenter membership; neutral defaults must erase work immediately. No production implementation is authorized by this scope-edit iteration.
3. Execute the local formula/failure/order/observer controls, narrow builds, native validation and both-backend package proof. Close the vignette sub-result separately from chromatic aberration inside `FCR-REN-25`; accepting one cannot accept the other.
