# HDR Display Output User Experience

**Status:** proposed product experience; blocked until `HDRD-00`, not an implemented UI or package claim

**Responsibility:** define request, discoverability, active/fallback truth, first use, transitions, UI-white behavior, errors, support, accessibility, and automation for HDR output

**Authority boundary:** [Execution Architecture](ExecutionArchitecture.md) owns runtime/native state; [Semantics](Semantics.md) owns color/luminance meaning; this page owns what a person and automation can observe and do

**Current readiness:** **0/100 — target only**.

## Product Priority Order

1. A usable coherent SDR image and truthful active/fallback state.
2. Generation-correct HDR activation and color/UI semantics.
3. Clear eligibility, transition, recovery, and evidence interpretation.
4. Responsive operation with bounded black/no-present time.
5. Advanced controls and presentation polish.

HDR availability never outranks output safety. An attractive HDR image on one monitor cannot justify a stale badge, broken SDR fallback, unreadable UI, or ambiguous profile.

## Product Pillar

**Request HDR once; know exactly what the current window is presenting; keep working when the display path cannot honor it.**

## Intended People And Jobs

| Person/job | Successful outcome | Not promised |
| --- | --- | --- |
| developer using an HDR display | request HDR, see current output/profile/active truth, and retain readable UI | automatic monitor calibration or universal HDR availability |
| rendering/color developer | inspect target/PQ/UI raw products and match them to native tuple/output generations | proving the panel from a compositor screenshot |
| backend/platform developer | reproduce capability/activation/fallback/transition state with native details | choosing artistic tone/gamut policy in RHI |
| release reviewer | execute one frozen D3D12/Vulkan/SDR/HDR/display/package matrix | accepting an OS badge, metadata call, or one backend as completion |
| support investigator | copy one bounded record and give an exact next action | asking users to interpret native handles or unbounded logs |

## Frozen Defaults And Operational Budgets

Stage 0 fills every row before implementation:

| Item | Required freeze |
| --- | --- |
| default request | SDR or product-configured HDR request, with persistence scope |
| automatic activation | whether eligible HDR activates immediately or requires confirmation/restart |
| profile labels | exact HDR10 UINT10/PQ and optional scRGB names; no ambiguous generic profile |
| peak/black/diffuse policy | fixed/adaptive/hybrid values, units, source validity, control/reset behavior |
| SDR UI white | current system query, bounds, fallback, update behavior, user control if any |
| transition presentation | continue old coherent profile versus bounded no-present; maximum black/no-present frames/time |
| automatic retry | event/rate/count/backoff and terminal error behavior |
| fallback persistence | whether request remains HDR while SDR is active and across restart/display move |
| support/capture | default artifacts, size/time/timeout/location/naming/sensitive-data rules |
| accessibility/safety | no flashing patterns; supported scale/input; brightness-warning policy if needed |

Values detected from the display are labeled `Reported` with validity/output identity. Creative/fallback values are labeled `Policy`. One may initialize the other only through a frozen rule.

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

## Setup And Automatic Preflight

The surface preflights current OS version/Advanced Color state, window/output association, backend/profile, required interface/extensions and eligible tuple, alpha/UI/interposer constraints, valid luminance/SDR-white facts or accepted fallback, shader/profile availability, and package configuration. Preflight reports `Supported`/`Eligible` facts; it does not report `Active` until the current native tuple and matching Renderer transform generation publish.

If Windows HDR is off or the window is on an SDR display, the interface gives the exact prerequisite and keeps the existing SDR path. It does not attempt hidden OS settings changes. If the window straddles displays, the accepted ownership rule and chosen output are visible.

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

## State, Dominant Action, And Pixels

| State | Current pixels/presentation | Dominant action | Other valid actions |
| --- | --- | --- | --- |
| `Off / SDR active` | known SDR transform and tuple | Request HDR | inspect support, capture SDR baseline |
| `Requested` | prior coherent profile, normally SDR | wait for preflight or cancel | inspect requested profile |
| `Unsupported` | SDR | satisfy named OS/display/backend prerequisite or Use SDR | copy support record |
| `Ineligible` | SDR | change window/profile/interposer condition or Use SDR | inspect exact cell/reason |
| `Activating/Revalidating` | explicitly named old coherent profile or bounded no-present state | Cancel/Use SDR | view elapsed budget and target output/profile |
| `Active HDR10` | matching Renderer HDR10 signal plus current HDR10 tuple | Use SDR | inspect policy/facts, capture, move/resize normally |
| optional `Active scRGB` | matching separate scRGB signal plus tuple | Use SDR | profile-specific details/capture only if admitted |
| `Fallback SDR` | matching known-good SDR transform plus tuple while HDR remains requested | Retry after prerequisite change or Use SDR | copy failure/transition lineage |
| `Error` | no valid presentation route; stale pixels are not claimed | Retry/close/recover owning device/window | copy diagnostics; no automatic retry storm |

The compact badge always names the active profile, not just the request. If requested and active differ, both are visible. `Active HDR` without exact profile/output generation is prohibited vocabulary.

## Visible Details And Controls

The default surface stays compact: request toggle, active/fallback summary, current display, and recovery. An advanced/support expander may show format, color space, backend, output generation, reported luminance validity/range, accepted target policy, current/fallback SDR white, metadata disposition, and last transition/error. Native handles, raw structs, or per-frame logs are not user vocabulary.

Advanced details separate:

- **Intent:** requested profile and persistence/product source;
- **Current output facts:** stable display label/identity, association generation, OS/Advanced Color state, reported primaries/luminance and validity, current SDR white and validity;
- **Native result:** backend, exact neutral format/color-space/profile, swapchain/device generation, eligibility/activation/fallback reason;
- **Renderer result:** semantic/target/UI policy generation, current profile transform, stage/product identity;
- **Metadata:** omitted/submitted/failed/unknown-effect and values/source, never folded into the active badge;
- **Last transition:** trigger, from/to profile/output generations, duration/black frames, rollback result.

Stable identifiers are copyable; raw native handles and full API structs remain private.

If `HDRD-04/05` admit user controls, their units are nits with valid ranges, current system/display values, reset-to-system/default behavior, and immediate requested-versus-active feedback. A fixed 1000/200 policy is labeled policy, not detected hardware fact.

Changing a policy creates a new Renderer semantic generation and may require revalidation; the UI shows authored/requested/effective values separately. A monitor move may update reported capability and system SDR white without rewriting the user's creative policy invisibly.

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

## Disable, Retry, Close, And Shutdown

| Action | Required behavior |
| --- | --- |
| Use SDR/disable | begin one bounded transaction to existing SDR transform/tuple; final state is `Off / SDR active` or explicit `Error` |
| cancel activation | invalidate candidate generation and retain/restore the declared coherent profile; racing completion cannot publish |
| retry | create a new transition generation after an event/manual action; never revive stale output facts |
| minimize | retain request, stop presenting zero extent through normal route, clear stale active-frame claim, revalidate on restore |
| close window/viewport | cancel eligibility/publication for that logical window and retire native/frame resources through owners |
| application shutdown | stop new transitions/retries, drain current native/frame ownership, and avoid modal prompts for invisible work |
| device/backend restart | show revalidating/unavailable, rebuild current output/tuple, and publish active only for new device generation |

## Transition Feedback And Guardrails

The transition surface is non-modal unless the existing application cannot remain usable. It names target profile/output, current coherent profile, elapsed versus maximum budget, and the action `Use SDR`. If the black/no-present budget is exceeded, the state becomes explicit fallback/error; it does not spin indefinitely.

No diagnostic or test journey requires flashing full-screen patterns. Peak/black/patch measurements use opt-in bounded fixtures with safety notice as appropriate and never run during ordinary first use.

## UI And Accessibility

SDR-authored UI remains legible at the accepted current/fallback white and does not clip, dim, or double encode. Status never relies on brightness or color alone. Text names `HDR requested`, `HDR active`, or `SDR fallback`; it does not use a single ambiguous checkbox. Controls are keyboard reachable, nit values and reasons are copyable, and no flashing transition pattern is required.

Display labels remain distinguishable for identical model names by a stable safe identifier without exposing native handles. Keyboard focus survives a bounded swapchain recreation where the UI host permits. At supported UI scales, the fallback/retry action and reason remain visible. Numeric input separates localized display from locale-independent manifest serialization.

## Capture And Support

Capture choices name their boundary: scene-working, target-linear HDR, PQ-encoded swapchain input, compositor screenshot, or external display measurement. The UI warns when a selected capture cannot validate physical display output. A bounded support record includes request/result, output/backend/OS/driver identity, active tuple, policy values and validity, transition generation, metadata disposition, and last failure.

## Artifact Experience

| Requested artifact | What the UI says it proves | Required warning/identity |
| --- | --- | --- |
| scene/target-linear raw | Renderer upstream/target values | not encoded/native/display proof; semantic/target generation |
| PQ or scRGB raw swapchain input | encoded signal values | not proof of selected native tuple or emitted light; exact profile/format |
| native state trace | format/color space/output/generation/call results | not proof of Renderer pixels or panel response |
| compositor screenshot | compositor-visible appearance symptom | capture path may transform/clip; never raw or physical measurement label |
| external capture/measurement/photo | bounded external observation | capture/meter/calibration/display/settings/environment/uncertainty required |

The evidence action creates a manifest before capture, writes artifacts transactionally, reports partial/missing outputs, and hashes complete results. It cannot label metadata success or monitor-reported peak as a physical measurement.

## Automation Equivalence

A manifest can request HDR or SDR, select backend/profile/window mode, require `Active` versus allow `Fallback SDR`, and request named raw/native/measurement artifacts. Machine output uses the same state/reason vocabulary and returns a non-success result when required HDR does not activate. Automation cannot force a backend to report active, bypass output association, or treat metadata success as proof.

Automation also freezes target window/display selection policy, timeout, allowed transition/black frames, peak/SDR-white policy, exact required profile, event/fault sequence, reference content, and cleanup. Results distinguish unsupported, ineligible, activation failure, fallback success, fallback failure, semantic mismatch, native mismatch, measurement inconclusive, and pass. An absent external meter/capture never becomes a pass.

## Build And Reachability Matrix

| Surface | Request/control | Native activation | Evidence/support |
| --- | --- | --- | --- |
| DevelopmentEditor | full admitted request/status and policy controls | only eligible profile/backend/window/UI cell | full developer artifact/support surface |
| noninteractive developer route | manifest only | same RHI/Renderer transaction | machine-readable full admitted artifacts |
| packaged Runtime | product/user request per release scope; no developer console dependency | same admitted backend/profile | bounded runtime status; clean-machine evidence route |
| release consumer UI | only explicitly product-approved control | no backend details/native forcing | no developer-only calibration/probe panel |
| unsupported/non-Windows/excluded profiles | no active claim | absent/ineligible/fallback | clear negative result, no selector advertisement |

## Stage-0 First-Use And Transition Dry Run

Independent reviewers walk: SDR baseline; HDR request with OS HDR off; eligible activation; metadata omitted/failure; UI-white change; resize/fullscreen/minimize; window straddle/move between HDR and SDR displays; OS toggle; hotplug; suspend/resume; interposer/interface loss; device/backend recovery; fallback success/failure; package first use; raw/native/compositor/external capture interpretation; and Use SDR. They inject stale and mixed generations in the paper/state model.

The dry run passes only when reviewers can identify current pixels/profile, current output and validity, why requested/active differ, next legal action, maximum wait/black period, and what each artifact proves without private explanation.

## Professional Defaults And Guardrails

- SDR remains the safe default/fallback unless product scope explicitly chooses otherwise;
- requesting HDR never changes OS settings silently;
- exact HDR10/scRGB profile names replace one ambiguous HDR toggle;
- reported display facts and creative/fallback policy are visually and semantically distinct;
- no metadata success, 10-bit format, Windows badge, brightness, screenshot, or responsive present is labeled active proof;
- UI white updates are generation-bound and never applied twice;
- automatic retries are bounded and stop at explicit error;
- display/driver support lists are evidence-scoped, not universal marketing claims;
- disabling HDR has a visible bounded route back to SDR even during activation/revalidation.

## Common Experience Failure Points

- a single checkbox shows on while the current tuple or Renderer transform is SDR;
- monitor move leaves stale output name/peak/SDR white or `Active` badge;
- HDR10 and scRGB results share the same label/capture interpretation;
- activation produces an indefinite black window without usable Use SDR action;
- fallback badge appears even though Renderer/native SDR halves did not both recover;
- UI is composed/encoded in the wrong domain and becomes dim, clipped, or overbright;
- metadata failure is reported as pixel failure, or success is reported as display proof;
- a compositor screenshot is offered as raw PQ or physical luminance evidence;
- the editor works only with a specific interposer/bypass while packaged Runtime differs;
- support output lacks exact output/profile/generation/validity/native reason;
- automation allows fallback while the requested contract required active HDR.

## Experience Exit

First use, enable/disable, unsupported/ineligible, activation failure, monitor move, OS toggle, resize/fullscreen, suspend/resume, device recovery, SDR-white update, UI appearance, capture interpretation, support output, package, and automation all need binary predeclared checks. A successful settings toggle or one HDR photograph is insufficient.

## UX Review And Acceptance Handoff

The UX handoff includes frozen defaults/budgets; profile/state/action/reason vocabulary; control-to-semantic/native mapping; first-use/transition/fallback dry-run transcripts; display-fact/policy distinction; UI-white and accessibility review; editor/manifest/package reachability; artifact examples and warnings; support-record schema; and controlled failures for stale/mixed state, black timeout, bad UI composition, false fallback, profile confusion, and artifact misclassification.

The experience passes only when all visible truth derives from [Execution Architecture](ExecutionArchitecture.md)'s immutable result/generation owners, controls match [Semantics](Semantics.md), SDR remains usable across admitted failures, automation matches the editor/product contract, and excluded routes remain absent. UI screenshots or documentation do not prove native or display behavior.

## Sources And Precedent

The experience uses the current Microsoft Advanced Color and SDR-white guidance recorded in [Research](Research.md#source-ledger) to frame profile eligibility, dynamic output changes, and reported-versus-policy wording. That guidance is precedent/constraint, not proof that Sparkle currently supports HDR or that one monitor/profile will pass.
