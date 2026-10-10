# External Capture User Experience

**Status:** target architecture; launch, viewport, automation and recovery contract

**Scope:** make the prioritized external tools discoverable and usable without exposing SDK or native-handle mechanics in normal user flows.

**Authority boundary:** runtime truth belongs to [Semantics](Semantics.md) and [Execution Architecture](ExecutionArchitecture.md); this page owns interaction. The parent [Visual Design](../VisualDesign.md) supplies layout precedent, not provider capability proof.

**Current implementation:** DebugEditor/DevelopmentEditor have single-provider startup selection, native bootstrap, request control and a viewport overlay. Scoped native results are recorded in [Discovery](Discovery.md#direct-launch-implementation-handoff---2026-10-10); lifecycle, full matrix, Game, Shipping product and independent adoption acceptance remain open. This source refresh does not increase readiness.

## Implemented Launch And Viewport Action

DebugEditor and DevelopmentEditor accept one `--capture-provider none|nsight-graphics|pix|renderdoc` selection (separated or equals form), or one case-insensitive `-AttachPix`, `-AttachNSight`, or `-AttachRenderDoc` alias. Explicit intent overrides the saved startup enum, including explicit None. Missing/invalid values, duplicate selections and unknown attachment flags reject before graphics creation. No live setting injects another tool.

The selected native activity bootstraps before intercepted graphics work. One recognizable application icon overlays the scene surface at its upper right with a two-font-height hit area. None contributes no overlay. The button submits the existing typed request; readiness and pending/quarantined state determine whether it is enabled. Marker availability alone cannot enable capture. The generic viewport owns overlay placement; the capture client owns presentation, and the shared icon service owns assets/resources. [Current composition](ExecutionArchitecture.md#implemented-editor-viewport-composition) and [native handoff](Discovery.md#direct-launch-implementation-handoff---2026-10-10) supersede the earlier disabled-artwork slice.

PIX and Nsight Graphics have D3D12 adapters; RenderDoc has D3D12 and Vulkan lowering. Nsight Vulkan and DevelopmentGame remain delivery work. Shipping excludes the optional route in source/build membership; full product/package erasure still needs acceptance evidence.

## Current Startup Selection

Select **GPU Capture** beside **Graphics API** in the Sparkle Launcher footer: **None -> Nsight Graphics -> PIX -> RenderDoc**. Installed eligible tools are selectable; supported missing tools remain visible with setup details, and incompatible API/profile/hardware choices remain disabled with reasons. Default None launches without optional hooks. Installation detection performs no injection and does not imply native readiness. Nsight's experimental status stays visible.

Direct IDE/shortcut launches may persist the same enum under **Window -> Settings -> GPU Capture -> Capture tool on startup** and restart. Explicit attachment flags or Launcher intent override that preference for one launch. One active provider owns capture; the larger icon overlays the viewport surface at its top right. Click it after the desired scene is ready. Capture applies to the containing host interval and includes Editor UI. Native finalization determines completion; pending/quarantined controls remain disabled. Rendering Settings, Launcher and UI never own a second native session.

## First Use: PIX

1. Select DevelopmentEditor and D3D12 in Launcher, then PIX under GPU Capture. Runtime verifies the activity; installation detection alone does not prove readiness. DevelopmentGame is still pending Stage 2.
2. Launch the level-run operation. Unsafe partial injection rejects launch; unavailable capability reports a reason without substituting another tool or graphics API.
3. Open the intended scene viewport and let it render. The PIX icon overlays the top-right scene surface; it captures the containing host interval, including Editor UI.
4. Hover for `Capture a GPU frame with PIX` and any pending or failure reason. Click once when enabled. The same authority rejects a second active request; the viewport retains its camera, settings and selection.
5. The current overlay shows `Capturing...` for queued/capturing work and the authority's message for unavailable, failed or quarantined work. Native finalization and handoff determine Completed; scheduling success does not.
6. The current adapter hands a finalized capture to the native tool. Artifact path, request, host-frame and context identity are retained in the neutral observation; the icon is not a detailed result browser.

### Remaining Presentation And Acceptance

The following are target requirements, not controls already present in the overlay: a detailed result view with explicit identity certainty and observer/provenance data; separate `Open in tool` and `Show in folder` actions where an accessible artifact exists; accessible status/recovery and narrow-layout behavior. Keep native handoff separate from capture finalization, and never invent a precise tool frame ID from the host scheduling boundary. [Remaining delivery](Plan.md) owns their implementation/acceptance disposition.

Install/update the external tool through its normal vendor workflow. Sparkle does not silently download or elevate a tool, alter driver settings, scan arbitrary directories, or recreate a graphics device from a capture button. Optional trusted installation selection lives in setup/advanced preferences only when automatic supported discovery fails.

## Nsight, RenderDoc, And Specialist Journeys

The same context action and authoritative operation apply to Nsight Graphics Capture and RenderDoc on supported backends. Nsight carries an `Experimental SDK` label even when its local cell is accepted. Their setup instructions name actual tool/activity/API prerequisites and explain that Graphics Capture differs from GPU Trace or Systems.

For specialist investigations, use `Open profiling guidance` with a selected question/marker: CPU scheduling -> WPA/Systems; D3D12 multi-frame timing -> PIX Timing; NVIDIA GPU pressure -> GPU Trace; crash -> Aftermath; pacing -> PresentMon. AMD RGP/RMV/RRA/RGA/uProf/RGD are excluded by the user scope decision on 2026-10-09 and are not guidance/context actions in this delivery. These entries open guidance or a supported external workflow, not an in-engine claim of readiness/capture completion. Tool-managed launch must preserve the actual product executable, working directory, level/API/settings and candidate identity, rather than launch the Launcher and assume child-process injection works.

## UI, CLI, And Game Equivalence

| Entry | Intent | Authority and observable output |
| --- | --- | --- |
| Launcher optional tools | One typed process-start provider selection | Same normalization as direct CLI; startup capability/reason comes from launched process. |
| Direct CLI | One `--capture-provider <id>` selection or attachment alias | Implemented Editor bootstrap and capture request route. Duplicate/unknown intent rejects before graphics; no tool-specific command string comes from Editor. |
| Editor viewport icon | Named provider + current stable scene-view target + next eligible frame | Existing Renderer control route, one request ID, immutable result model. |
| DevelopmentGame (remaining) | Same single-provider intent and existing development console/control operation | Stage 2 must add and validate the Game route; the current Editor-only implementation does not enable it. Provider-native capture is a distinct external workflow. |
| Automation | Same typed request and observed result; explicit readiness precondition and deadline | No script clicks a vendor icon or scrapes filename existence to infer completion. Nonzero rejection/failure result and retained native artifact identity. |

Process-start selection cannot change on a running graphics context. UI offers `Relaunch with tool` guidance; unsaved Editor state follows existing save/relaunch workflow, with no automatic destructive restart.

## Target State And Error Presentation

| State/failure | Visible behavior / next action |
| --- | --- |
| No selected provider | No vendor capture group. |
| Unavailable/unsupported/conflicting | Disabled named action, exact reason and setup/relaunch link; no selectable fallback. |
| Ready | Enabled named action and containing target; no promise of viewport-only contents. |
| Submitted/Queued | Acknowledge pending execution without claiming the SDK is Armed; no second click creates a backlog. |
| Armed | Target and request preserved; cancellation offered only when the runtime can safely withdraw. |
| Capturing/Finalizing | Text status; no blocking progress dialog, invented percentage, or implied cancel support. |
| Busy | Names the active provider; the selected overlay remains visible. |
| Queue full/closing | Immediate readable rejection; keep the viewport usable and existing result intact. Retry when capacity returns; closing follows existing shutdown behavior. This is distinct from a native capture being Busy. |
| Timeout/draining | Request failure plus `Native capture still draining; relaunch required` when quiescence is unknown; no Retry action that can overlap. |
| Target resized/destroyed/not presenting | Exact target failure; restore/reopen view and request again after safe settlement. |
| Artifact not accessible/open failed | Distinguish capture result from open action failure; manual native-tool open when appropriate. |
| Device loss/shutdown | Existing terminal device policy, safe detached UI and one request settlement; no capture-specific device-recovery promise. |

Errors follow `Problem`, `Expected`, `Observed`, `Why it matters`, `Next action`, and bounded `Details`. User text says “PIX is not attached for GPU capture” rather than an HRESULT alone. Details retain the native error and support identity without becoming another log stream.

## Target Accessibility And First-Use Gate

Every action has provider/activity accessible name, keyboard focus, readable state/reason and non-color status. Use existing UI style/DPI policy; small glyphs have the usual hit target. At narrow widths collapse into a labeled `Capture` menu with the same entries/state rather than hide failures. Exercise supported scaling/high-contrast/keyboard routes and paths containing spaces/Unicode. Units and native target scope remain readable; serialized support identity is locale-independent.

`CHK-EC-UX` and `CHK-EC-ADOPT` retain clean first-use, failed setup, Busy, stale target, timeout/drain, open failure, automation and non-author recovery transcripts. Screenshots verify layout/state presentation only. Runtime capture, symbols, native validation, observer cost and Shipping proof require the separate dossier checks.
