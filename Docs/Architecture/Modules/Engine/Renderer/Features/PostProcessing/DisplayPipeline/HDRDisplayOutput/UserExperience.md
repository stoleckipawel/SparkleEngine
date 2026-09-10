# HDR Display Output User Experience

**Status:** proposed product experience; blocked until `HDRD-00`, not an implemented UI or package claim

**Responsibility:** define request, discoverability, active/fallback truth, first use, transitions, UI-white behavior, errors, support, accessibility, and automation for HDR output

**Authority boundary:** [Execution Architecture](ExecutionArchitecture.md) owns runtime/native state; [Semantics](Semantics.md) owns color/luminance meaning; this page owns what a person and automation can observe and do

**Current readiness:** **0/100 — target only**.

## Product Promise

On an admitted Windows/display/backend/profile cell, a person can request HDR, see whether it is eligible and actually active on the current output, and receive the intended HDR10 image with readable SDR-authored UI. On every unsupported or failed cell, Sparkle keeps a usable SDR image and explains why HDR is not active.

The product does not promise HDR merely because Windows HDR is enabled, the back buffer is 10-bit, metadata was submitted, or a screenshot looks bright.

## First Use

1. Open the existing rendering/display settings surface.
2. See **HDR Output** with current request, backend, display identity, OS/Advanced Color state, eligibility, active/fallback result, signal tuple, target peak policy, and SDR white policy.
3. Request HDR. If a presentation recreation is required, the UI states that transition before applying it.
4. Sparkle activates the complete tuple or returns atomically to SDR with a stable reason and recovery action.
5. Move the window, resize, toggle OS HDR, or change display; the state changes through `Revalidating` rather than leaving stale `Active` truth.
6. **Use SDR** returns to the known-good SDR route and persists the request according to the accepted product policy.

## State Vocabulary

| State | Meaning |
| --- | --- |
| `Off / SDR active` | HDR is not requested and the known SDR tuple is active |
| `Requested` | user/product intent exists; support is not yet claimed |
| `Unsupported` | current OS/display/backend/profile lacks a required capability |
| `Ineligible` | capabilities exist but the current window/composition/interposer tuple cannot use the admitted route |
| `Activating` | recreation/state transition is in progress; old active result remains explicit |
| `Active` | current output generation has the complete accepted native tuple and matching Renderer transform |
| `Fallback SDR` | HDR remains requested but SDR is active after unsupported state or failure, with reason |
| `Error` | neither valid presentation route is currently available; never display a plausible stale HDR frame |

## Visible Details And Controls

The default surface stays compact: request toggle, active/fallback summary, current display, and recovery. An advanced/support expander may show format, color space, backend, output generation, reported luminance validity/range, accepted target policy, current/fallback SDR white, metadata disposition, and last transition/error. Native handles, raw structs, or per-frame logs are not user vocabulary.

If `HDRD-04/05` admit user controls, their units are nits with valid ranges, current system/display values, reset-to-system/default behavior, and immediate requested-versus-active feedback. A fixed 1000/200 policy is labeled policy, not detected hardware fact.

## Transitions And Recovery

| Journey | Required experience |
| --- | --- |
| unsupported display / OS HDR off | request remains understandable; SDR stays usable; exact prerequisite and action shown |
| monitor move / output association change | state becomes Revalidating; new output identity and final active/fallback reason shown |
| fullscreen/window/resize | bounded transition; no indefinite black frame; final tuple/state visible |
| color-space/recreate failure | automatic SDR recovery, one actionable error, retry after prerequisite changes |
| metadata failure | behavior follows accepted optional/required policy and never claims metadata changed pixel interpretation |
| suspend/resume/device recovery | stale active badge clears until current tuple is restored |
| packaged missing capability | same structured reason as editor; no debug-console prerequisite |

## UI And Accessibility

SDR-authored UI remains legible at the accepted current/fallback white and does not clip, dim, or double encode. Status never relies on brightness or color alone. Text names `HDR requested`, `HDR active`, or `SDR fallback`; it does not use a single ambiguous checkbox. Controls are keyboard reachable, nit values and reasons are copyable, and no flashing transition pattern is required.

## Capture And Support

Capture choices name their boundary: scene-working, target-linear HDR, PQ-encoded swapchain input, compositor screenshot, or external display measurement. The UI warns when a selected capture cannot validate physical display output. A bounded support record includes request/result, output/backend/OS/driver identity, active tuple, policy values and validity, transition generation, metadata disposition, and last failure.

## Automation Equivalence

A manifest can request HDR or SDR, select backend/profile/window mode, require `Active` versus allow `Fallback SDR`, and request named raw/native/measurement artifacts. Machine output uses the same state/reason vocabulary and returns a non-success result when required HDR does not activate. Automation cannot force a backend to report active, bypass output association, or treat metadata success as proof.

## Experience Exit

First use, enable/disable, unsupported/ineligible, activation failure, monitor move, OS toggle, resize/fullscreen, suspend/resume, device recovery, SDR-white update, UI appearance, capture interpretation, support output, package, and automation all need binary predeclared checks. A successful settings toggle or one HDR photograph is insufficient.

