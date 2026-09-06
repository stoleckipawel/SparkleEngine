# Debug View Presentation — Acceptance

Status: feature-local acceptance contract; not proof that debug-view presentation has passed

Scope: feature-local acceptance for mode classification, show-flag resolution, presentation mapping, viewport isolation, capture metadata, backend parity, and repository checks

Feature architecture: [View Modes And Show Flags](ViewModesAndShowFlags.md) and [Debug View Presentation Architecture](PresentationArchitecture.md)

Delivery authority: [Debug View Presentation Delivery Plan](../../../../../../Plans/Renderer/DebugViewPresentation.md)

Release reporting authority: [Feature Completion Reports](../../../../../../Acceptance/FeatureCompletionReports.md)

This file is part of the Debug Views feature dossier and owns the proof contract required after delivery. Candidate results remain in the release-level completion report and must retain exact commands, configurations, artifacts, and limitations.

## Traceability And Current Disposition

| Dimension | Binding target |
| --- | --- |
| North Star | `NS-REAL`, `NS-MATH-DATA`, `NS-EVIDENCE`, `NS-OWNERSHIP`, `NS-ADOPTION`, `NS-SIMPLIFY` |
| Persona targets | `PGE-01`, `PGE-06`, `PGE-07`, `PGE-13` |
| Roadmap targets | Classify at `REL-00`; if included, close feature polish at `REL-04`, visual quality at `REL-05`, and advertised backend parity at `REL-07`. |
| Feature completion | `FCR-REN-11`; current state remains `Source present`/`Blocked` until a candidate report closes the criteria below. |
| Release risks | `RISK-REL-05`, `RISK-REL-06`, `RISK-REL-08`, `RISK-REL-11` |

| Risk seed | Cause, event, and consequence | Current likelihood / impact | Owner / gate | Prevention and detection; contingency and retirement evidence |
| --- | --- | --- | --- | --- |
| `RISK-DVP-01` | Producer-local display mapping plus shared presentation applies exposure, tone mapping, or encoding twice and makes diagnostics numerically misleading. | High / High | Renderer feature owner; `REL-04`, `REL-05` | One signal-domain/presentation contract and fixed numeric checks. If unresolved, make affected views unreachable. Retire only when every declared combination meets tolerance. |
| `RISK-DVP-02` | Process-global mode or mutable editor state crosses the viewport boundary, producing races, cross-talk, or non-reproducible captures. | High / High | View/editor owners; `REL-04` | View-request ownership, one resolver, dual-viewport and rapid-toggle challenge. If unresolved, exclude multi-viewport/custom overrides. Retire with isolation and replay evidence. |
| `RISK-DVP-03` | Unavailable resources, stale outputs, or incomplete metadata present a plausible image that cannot be trusted or replayed. | High / High | Capture/evidence owners; `REL-04`, `REL-05` | Explicit unavailable state, generation identity, complete sidecar, and deliberate verifier-detection control. Reject incomparable captures; retire with negative-check evidence. |
| `RISK-DVP-04` | Backend, format, extent, or output-encoding differences change the meaning of a named view. | Medium / High | Renderer/RHI owners; `REL-07` | Predeclared formats/tolerances, resize/output matrix, paired native validation, decoded-pixel comparison. Exclude a failing backend/mode combination; retire only for advertised rows. |

These are seeds for the live risk ledger in the `FCR-REN-11` candidate report; only that report records current disposition or retirement. The owning iteration record maps every applicable `AC-DVP-*` and `FM-DVP-*` below to the named `CHK-DVP-*`, retained artifacts, and a result. These IDs define claims and checks; they do not assert that any check has run.

## Completion Criteria

Implementation is accepted only when all of the following are demonstrated:

- `AC-DVP-01` — every `RenderViewMode` other than `Count` has exactly one signal domain and one explicit show-flag preset;
- `AC-DVP-02` — every `RenderShowFlag` other than `Count` has exactly one metadata entry, a real producer/consumer path, deterministic disabled behavior, and classified graph impact;
- `AC-DVP-03` — no generic `RenderFeatureFlags` definition is reintroduced; selection, view-mode presets, requested outputs, and show flags each have one target representation;
- `AC-DVP-04` — the active mode and show-flag overrides arrive through the viewport/view request; normal pass behavior does not read `CVarRenderViewMode` or another process-global substitute;
- `AC-DVP-05` — `RenderViewBuilder` is the only resolver, and two viewports can resolve different show-flag sets without global-state races or cross-talk;
- `AC-DVP-06` — stock `Lit`, `GBufferEmissive`, and direct/indirect lighting modes enable Exposure and Tonemapper; changing exposure compensation or tone mapper changes them;
- `AC-DVP-07` — those stock HDR modes contain no producer-local display curve and are tone mapped once;
- `AC-DVP-08` — stock roughness, metallic, ambient occlusion, subsurface strength, normals, diffuse/subsurface material colors, and instance palette views disable Exposure and Tonemapper; changing exposure compensation or tone mapper does not change their decoded display-linear pixels;
- `AC-DVP-09` — toggling either presentation flag takes effect on the next submitted frame, marks the viewport Custom, and Reset Show Flags restores the stock mode result;
- `AC-DVP-10` — all four `Exposure`/`Tonemapper` combinations match their specified behavior, including intentional linear-path clipping for HDR values;
- `AC-DVP-11` — changing `Sky`, `DirectLighting`, `IndirectLighting`, `Shadows`, `DebugOverlay`, and `GizmoOverlay` affects only the current viewport and matches each flag's documented disabled behavior;
- `AC-DVP-12` — sRGB and linear output configurations both produce the expected displayed values without double encoding;
- `AC-DVP-13` — exact views do not pass through temporal reconstruction, sharpening, bloom, color grading, or future scene post effects;
- `AC-DVP-14` — exposure continues to meter the lit scene while an exact view is active, and returning to `Lit` does not reset adaptation history;
- `AC-DVP-15` — render/output extent mismatch has an explicit point-sampling result with no out-of-bounds reads;
- `AC-DVP-16` — captured viewport-product metadata and replay record the view kind, mode, resolved flags, override deltas, presentation values, output encoding, and any CVar force; stock and Custom captures are distinguishable;
- `AC-DVP-17` — no runtime pass reads editor state, resolves a flag by string, or independently consults a CVar for behavior already represented by the resolved show-flag set;
- `AC-DVP-18` — the Show menu exposes only implemented flags, grouped by purpose, with working reset/category actions and no raw bit/CVar UI;
- `AC-DVP-19` — documentation does not describe the encoded viewport image as a raw GBuffer dump;
- `AC-DVP-20` — representative D3D12 and Vulkan results agree within the target format's quantization tolerance;
- `AC-DVP-21` — `git diff --check`, the selected shader cook, focused tests, and the required backend smokes report exact commands and results.

## Failure Modes And Key Checks

| Failure ID | Controlled failure or challenge | Accepted behavior | Covering check |
| --- | --- | --- | --- |
| `FM-DVP-01` | Select an HDR mode and an exact display-linear mode while varying exposure and tone mapper. | HDR responds exactly once; exact values remain invariant apart from the declared output encoding. | `CHK-DVP-03` |
| `FM-DVP-02` | Give two simultaneous viewports different modes, overrides, sizes, and rapid toggle/reset sequences. | Each viewport resolves independently on the next submitted frame with no race, cross-talk, stale flags, or history reset. | `CHK-DVP-02` |
| `FM-DVP-03` | Select a view whose producer is unavailable or disabled. | The viewport reports unavailable state or the specified deterministic disabled result; it never presents stale or unrelated data as success. | `CHK-DVP-02`, `CHK-DVP-04` |
| `FM-DVP-04` | Resize across render/output extent mismatch and switch sRGB/linear output. | Sampling stays in bounds and the declared mapping/encoding occurs once without clipping beyond the stated linear-path policy. | `CHK-DVP-03` |
| `FM-DVP-05` | Replay a capture with missing, inconsistent, or forced-mode metadata. | Verification rejects the mismatch and identifies mode, flags, presentation, encoding, and force source; it does not compare incomparable images. | `CHK-DVP-04` |
| `FM-DVP-06` | Exercise representative modes on both advertised backends. | Results remain within the predeclared tolerance and native diagnostics contain no uncategorized issue. | `CHK-DVP-05` |

| Check ID | Key test element and oracle | Coverage |
| --- | --- | --- |
| `CHK-DVP-01` | Enumerate every mode/flag and trace its one owner, preset/metadata row, producer/consumer, disabled behavior, and graph impact; reject omissions, duplicates, generic flag bags, and runtime global reads. | `AC-DVP-01`–`AC-DVP-04`, `AC-DVP-17`, `AC-DVP-19` |
| `CHK-DVP-02` | Run a dual-viewport interaction sequence over selection, custom overrides, reset, unavailable producers, and return to Lit; compare resolved state on each submitted frame. | `AC-DVP-05`, `AC-DVP-09`, `AC-DVP-11`, `AC-DVP-14`, `AC-DVP-18`; `FM-DVP-02`, `FM-DVP-03` |
| `CHK-DVP-03` | Use fixed numeric/reference inputs for every signal domain across four presentation combinations, sRGB/linear output, and mismatched extents; compare decoded pixels to predeclared values/tolerance. | `AC-DVP-06`–`AC-DVP-08`, `AC-DVP-10`, `AC-DVP-12`, `AC-DVP-13`, `AC-DVP-15`; `FM-DVP-01`, `FM-DVP-04` |
| `CHK-DVP-04` | Capture and replay stock/custom/unavailable states; schema-validate identity and deliberately remove or alter one field to prove the verifier detects it. | `AC-DVP-16`; `FM-DVP-03`, `FM-DVP-05` |
| `CHK-DVP-05` | Run the selected cook and focused D3D12/Vulkan smokes with native validation, then compare representative decoded outputs within quantization tolerance and retain exact commands/artifacts. | `AC-DVP-20`, `AC-DVP-21`; `FM-DVP-06` |
