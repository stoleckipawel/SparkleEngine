# HDR Display Output Discovery

**Status:** acceptance contract; `HDRD-00` is `Blocked`, and production implementation is not authorized

**Responsibility:** freeze the Windows HDR product, scene/display semantics, target and SDR-white policy, D3D12/Vulkan presentation contract, transitions, UX, budgets, and evidence before `DSP-7`

**Authority boundary:** [Research](Research.md) owns external findings; [Semantics](Semantics.md), [Execution Architecture](ExecutionArchitecture.md), and [User Experience](UserExperience.md) are conditional candidates; [Plan](Plan.md) orders delivery; [README](README.md) owns `AC-HDR-*`/`FM-HDR-*`; RHI owns native mechanics, not display policy

**Verified:** 2026-09-10 against committed revision `669637cf`; Renderer/RHI/settings/window/swapchain/UI/capture/package source inspected; no executable HDR hardware evidence was run

**Current readiness:** **0/100** — discovery and research add no readiness credit. See [Current Feature Readiness](../../../../../../../../Acceptance/CurrentReadiness.md#renderer).

HDR is not established by a 10-bit texture, a PQ function, a metadata call, or a monitor badge. The accepted route must join one color/luminance contract to an actually activated output tuple and remain truthful through window/display/device transitions and every fallback.

## Why The Existing Target Needs Discovery

The current first-release contract selected a fixed 1000-nit HDR10 target, 200-nit SDR UI white, a 10-bit swapchain, and static metadata. Current Microsoft guidance adds material constraints: FP16/scRGB is the general Advanced Color route, the UINT10/PQ route is suitable only for narrower cases; applications should use the system SDR white level when composing their own UI; and applications should not rely on swapchain metadata being honored. Khronos likewise states that metadata does not choose color space and its use is outside Vulkan's control.

Those findings do not cancel the admitted HDR10 goal. They make fixed target/white, UINT10 eligibility, and metadata role explicit `HDRD-*` decisions instead of settled implementation details.

## Current Source Truth

At `669637cf`:

- [`PixelFormat.h`](../../../../../../../../../Engine/RHI/Public/Formats/PixelFormat.h) has no 10-bit packed RGB format.
- [`RhiPresentationDefaults.h`](../../../../../../../../../Engine/RHI/Public/Presentation/RhiPresentationDefaults.h) admits four 8-bit SDR back-buffer formats only.
- [`RhiPresentationService.h`](../../../../../../../../../Engine/RHI/Public/Presentation/RhiPresentationService.h) exposes current present format but no output capability, color space, luminance, metadata, or requested/active/fallback result.
- [`D3D12SwapChain.cpp`](../../../../../../../../../Engine/RHI/Private/D3D12/SwapChain/D3D12SwapChain.cpp) creates/resizes/presents a format-selected flip swapchain without output/color-space/HDR policy.
- [`VulkanSwapChain.cpp`](../../../../../../../../../Engine/RHI/Private/Vulkan/SwapChain/VulkanSwapChain.cpp) accepts only the requested format paired with `VK_COLOR_SPACE_SRGB_NONLINEAR_KHR`.
- [`Presentation.cpp`](../../../../../../../../../Engine/Renderer/Private/Passes/Presentation/Presentation.cpp) tone maps, encodes, copies, and publishes an SDR-oriented product; UI is composed through a separate RHI overlay route.

This is a verified source-backed absence. It does not establish which Advanced Color route is correct for Sparkle or that the host/UI composition can use a specific swapchain tuple.

## Blocking Decisions

| ID | Decision | Required disposition |
| --- | --- | --- |
| `HDRD-01` | product and profile | freeze supported Windows versions, window/fullscreen behavior, DevelopmentEditor and packaged Runtime scope, D3D12/Vulkan cells, and explicit scRGB/HLG/dynamic-metadata exclusions |
| `HDRD-02` | scene-working contract | name Renderer scene primaries/white/units, exposure relation, reference diffuse white, legal range, and conversion into the HDR output transform |
| `HDRD-03` | output approach | freeze the packed 10-bit Rec.2020/PQ route required to satisfy the admitted HDR10 result; decide whether any DevelopmentEditor/UI profile additionally uses FP16/scRGB or is explicitly ineligible, and state fallback per profile/backend without calling the two signal encodings equivalent |
| `HDRD-04` | target luminance | ratify fixed 1000-nit mastering, adapt tone mapping to queried output luminance, or define a bounded hybrid; freeze black/peak/FALL policy and user control |
| `HDRD-05` | SDR/UI white | replace or ratify fixed 200 nits against the current system/user SDR white query; define 80-nit reference conversion, update events, bounds, and fallback |
| `HDRD-06` | gamut/tone/PQ semantics | freeze scene-to-Rec.2020 transform, gamut mapping, tone curve, clipping, PQ constants/precision, alpha, dithering, and exactly-once ownership |
| `HDRD-07` | D3D12 activation | freeze output association/capability query, format, color-space support/set, swapchain interface/interposer compatibility, metadata policy, result reporting, and recreation order |
| `HDRD-08` | Vulkan activation | freeze required instance/device extensions, surface format/color-space selection, metadata availability/policy, Win32 presentation behavior, result reporting, and recreation order |
| `HDRD-09` | metadata role | decide whether static metadata is omitted, diagnostic/best-effort, or required; it may never define pixel interpretation or serve as active-HDR proof |
| `HDRD-10` | state and transitions | freeze requested/supported/eligible/activating/active/fallback/error states and display/OS/window/resize/fullscreen/suspend/device/interposer invalidation events |
| `HDRD-11` | UI/capture/debug | freeze SDR UI mapping/composition, debug-view behavior, raw/display/capture identity, screenshot limitations, and observable support output |
| `HDRD-12` | evidence and budgets | freeze HDR/SDR display matrix, measurement/reference patterns, backend rules, transition/fault cases, black/peak/color tolerances, performance/memory/latency budgets, artifacts, and escalation |

## Required Discovery Experiments

| Experiment | Question | Required retained result |
| --- | --- | --- |
| `HDR-EXP-01` current domain/owner trace | where do scene units become display nits and who owns each transform/API state? | one Renderer -> RHI -> OS/display diagram with format/domain/state at every edge |
| `HDR-EXP-02` Windows route probe | which current Sparkle window/UI/profile cells are eligible for UINT10/PQ versus FP16/scRGB? | D3D12 capability/output/alpha/composition/interposer matrix on named Windows versions |
| `HDR-EXP-03` Vulkan route probe | are HDR10 surface color space and metadata extension available and behaviorally usable on the release machine? | extension/surface-format/present/monitor matrix with exact driver/OS/runtime identity |
| `HDR-EXP-04` luminance policy study | how do fixed 1000/200 targets compare with output-reported peak and current system SDR white? | measured/query table, candidate tone/UI mappings, decision rationale, and failure bounds |
| `HDR-EXP-05` semantic oracle | do PQ/gamut/tone/UI transforms match independent high-precision reference values and expose double/omitted transforms? | frozen ramps, patches, matrices, tolerances, and seeded defects |
| `HDR-EXP-06` transition model | can every monitor/OS/window/resize/fullscreen/suspend/device/interposer event reach valid active or SDR fallback state? | state machine with injected query/set/recreate/metadata failures and cleanup |
| `HDR-EXP-07` experience walkthrough | can a person identify active versus requested HDR and recover without a black/washed output? | first-use, unsupported, monitor move, OS toggle, fallback, support, and package journeys |
| `HDR-EXP-08` evidence dry run | can the protocol distinguish correct signal values, OS/display processing, screenshot capture, and visual appearance? | artifact lineage and minimum hardware/measurement matrix with invalid/ambiguous verdict rules |

## Risk Register

| ID | Cause, event, consequence | Prevention/detection | Contingency, owner, retirement |
| --- | --- | --- | --- |
| `RISK-HDR-01` | unnamed scene primaries/units make a numerically valid PQ result colorimetrically wrong | `HDRD-02/06`, independent matrices/ramps | block transform; Renderer display owner retires after semantic oracle passes |
| `RISK-HDR-02` | format is created but color space is unsupported/not set, producing false active state or washed output | tuple-based activation and native inspection | recreate known-good SDR and report reason; RHI owner retires after fault/transition checks |
| `RISK-HDR-03` | fixed 200-nit UI ignores the user's current SDR white, making UI too dim/bright | `HDRD-05`, system query and measured UI patch | use accepted bounded fallback and remain explicit; platform/display owner retires after update/failure proof |
| `RISK-HDR-04` | metadata is trusted as signal interpretation or physical-display proof | `HDRD-09`; pixel/color-space/measurement evidence separate | treat metadata as optional state; acceptance owner retires after checks demonstrate independence |
| `RISK-HDR-05` | display/OS/window/device changes leave stale active state or black output | explicit state machine and event invalidation; fault injection | atomically return to SDR or no-frame safe state; RHI/window owner retires after matrix |
| `RISK-HDR-06` | D3D12 interposer or Vulkan extension path diverges from the native owner contract | explicit interface/capability cells and architecture boundary check | block affected backend/profile; owner retires after native and paired semantic evidence |
| `RISK-HDR-07` | screenshots/captures are interpreted as display evidence despite compositor/display transforms | raw encoded values, native state, external measurement, artifact labels | verdict `Inconclusive`; capture/acceptance owner retires after lineage review |
| `RISK-HDR-08` | HDR expansion destabilizes proven SDR output | stage-by-stage SDR preservation and automatic fallback checks | keep/default to known SDR tuple; display owner retires after complete transition and package matrix |

## Discovery Acceptance

| ID | Pass criterion | Evidence |
| --- | --- | --- |
| `AC-HDRD-01` | `HDRD-01` through `12` have reviewed dispositions with no implementation-shaping unknown. | decision table and invalidation triggers |
| `AC-HDRD-02` | every Renderer/RHI/OS/display edge has one owner, domain, format, identity, lifetime, failure, and observable state. | `HDR-EXP-01`, architecture and state-machine revisions |
| `AC-HDRD-03` | D3D12 and Vulkan have truthful eligible/unsupported cells on named release hardware/software. | `HDR-EXP-02/03` raw queries and limitations |
| `AC-HDRD-04` | tone/peak/black/gamut/PQ/UI-white/metadata policies are numerically defined and consistent with current platform guidance or explicitly justified divergence. | `HDR-EXP-04/05`, accepted semantics |
| `AC-HDRD-05` | transition, fallback, UI, debug, capture, and support journeys are reviewable without interpreting intent. | `HDR-EXP-06/07`, accepted UX |
| `AC-HDRD-06` | every `AC-HDR-*`, `FM-HDR-*`, and material risk maps to a defect-detecting check with fixed matrix, threshold, artifact, cleanup, and budget. | `HDR-EXP-08` and no-orphan map |

`FM-HDRD-01` is any native or color decision deferred into an implementation prompt. `FM-HDRD-02` is any backend called active without a complete compatible tuple and current output association. `FM-HDRD-03` is reliance on metadata, screenshot, or visual plausibility as the sole oracle. `FM-HDRD-04` is an untested SDR fallback or transition that can leave no usable output. Any occurrence keeps the gate `Blocked`.

## Gate Decision

`HDRD-00` is **Blocked**. A future `PASS` must name the accepted dossier/research/semantics/architecture/experience/plan revisions and the exact release hardware/software cells. It authorizes only Stage 1 of [Plan](Plan.md); `FCR-REN-26` remains blocked until all feature evidence passes.
