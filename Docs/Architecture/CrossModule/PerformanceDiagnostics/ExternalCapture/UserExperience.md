# External Capture User Experience

**Status:** target architecture; launch, viewport, automation and recovery contract

**Scope:** make the prioritized external tools discoverable and usable without exposing SDK or native-handle mechanics in normal user flows.

**Authority boundary:** runtime truth belongs to [Semantics](Semantics.md) and [Execution Architecture](ExecutionArchitecture.md); this page owns interaction. The parent [Visual Design](../VisualDesign.md) supplies layout precedent, not provider capability proof.

**Current readiness:** **0/100 — target only**; no capture action exists in the audited baseline.

## First Use: PIX

1. Select the existing optimized Development Editor or Game product and D3D12 in Launcher. Choose PIX under optional capture tools; no tool is selected by default. The preflight names external installation/compiled-capability prerequisites, while runtime remains the readiness authority.
2. Launch the existing level-run operation. Bootstrap failure gives one provider-specific reason and recovery; unsafe partial injection fails launch. Nothing silently switches the graphics API or substitutes RenderDoc.
3. In Editor, open the intended scene viewport. The compact capture group is beside existing right-side camera/status controls in stable order **PIX, Nsight, RenderDoc**. Only requested/detected entries appear; requested unavailable entries remain disabled with readable setup guidance.
4. Read the action tooltip: `Capture next frame with PIX`, D3D12, scene/view context, containing present surface, readiness and observer warning. It states that the native capture is a host-frame interval that can include other work.
5. Click once. Show Armed, then Capturing/Finalizing only when observed. Other actions show Busy without disappearing. The viewport stays usable and keeps its current camera/settings/selection.
6. When confirmed complete, show `Open in PIX` and `Show in folder` for a finalized accessible artifact; for a verified native-UI handoff show that explicitly with `Path unavailable`. Offer opening as an explicit action by default to avoid stealing focus. Opening failure does not retroactively turn a valid capture into failure.
7. Show a compact result summary: provider/activity, scene/present target, request and actual frame/interval certainty, artifact or handoff, validation/observer settings. The details route carries version/hash/error data for support. Never invent an exact captured `FrameId` when the provider exposes only an interval.

Install/update the external tool through its normal vendor workflow. Sparkle does not silently download or elevate a tool, alter driver settings, scan arbitrary directories, or recreate a graphics device from a capture button. Optional trusted installation selection lives in setup/advanced preferences only when automatic supported discovery fails.

## Nsight, RenderDoc, And Specialist Journeys

The same context action and authoritative operation apply to Nsight Graphics Capture and RenderDoc on supported backends. Nsight carries an `Experimental SDK` label even when its local cell is accepted. Their setup instructions name actual tool/activity/API prerequisites and explain that Graphics Capture differs from GPU Trace or Systems.

For specialist investigations, use `Open profiling guidance` with a selected question/marker: CPU scheduling -> WPA/Systems; D3D12 multi-frame timing -> PIX Timing; NVIDIA GPU pressure -> GPU Trace; AMD queue/wave analysis -> RGP; memory -> RMV; RT structure -> RRA; shader ISA -> RGA; crash -> Aftermath/RGD; pacing -> PresentMon. These entries open guidance or a supported external workflow, not an in-engine claim of readiness/capture completion. Tool-managed launch must preserve the actual product executable, working directory, level/API/settings and candidate identity, rather than launch the Launcher and assume child-process injection works.

## UI, CLI, And Game Equivalence

| Entry | Intent | Authority and observable output |
| --- | --- | --- |
| Launcher optional tools | Typed process-start provider set | Same normalization as direct CLI; startup capability/reason comes from launched process. |
| Direct CLI | Repeat `--capture-provider pix`, `--capture-provider nsight-graphics`, or `--capture-provider renderdoc` for distinct requested entries | These are target flags, not current implemented commands. Invalid/duplicate/conflicting input rejects; no tool-specific string command from Editor. |
| Editor viewport icon | Named provider + current stable scene-view target + next eligible frame | Existing Renderer control route, one request ID, immutable result model. |
| DevelopmentGame | Same launch intent, existing typed development console/control operation for capture and current game target | Stage 2 freezes concise command grammar in existing command owner; no new game panel. Provider-native UI/hotkey capture is an additional explicit external workflow. |
| Automation | Same typed request and observed result; explicit readiness precondition and deadline | No script clicks a vendor icon or scrapes filename existence to infer completion. Nonzero rejection/failure result and retained native artifact identity. |

Process-start selection cannot change on a running graphics context. UI offers `Relaunch with tool` guidance; unsaved Editor state follows existing save/relaunch workflow, with no automatic destructive restart.

## State And Error Presentation

| State/failure | Visible behavior / next action |
| --- | --- |
| No requested/detected providers | No vendor capture group. |
| Unavailable/unsupported/conflicting | Disabled named action, exact reason and setup/relaunch link; no selectable fallback. |
| Ready | Enabled named action and containing target; no promise of viewport-only contents. |
| Armed | Target and request preserved; cancellation offered only when the runtime can safely withdraw. |
| Capturing/Finalizing | Text status; no blocking progress dialog, invented percentage, or implied cancel support. |
| Busy | Names active provider; retain other capability entries. |
| Timeout/draining | Request failure plus `Native capture still draining; relaunch required` when quiescence is unknown; no Retry action that can overlap. |
| Target resized/destroyed/not presenting | Exact target failure; restore/reopen view and request again after safe settlement. |
| Artifact not accessible/open failed | Distinguish capture result from open action failure; manual native-tool open when appropriate. |
| Device loss/shutdown | Existing terminal device policy, safe detached UI and one request settlement; no capture-specific device-recovery promise. |

Errors follow `Problem`, `Expected`, `Observed`, `Why it matters`, `Next action`, and bounded `Details`. User text says “PIX is not attached for GPU capture” rather than an HRESULT alone. Details retain the native error and support identity without becoming another log stream.

## Accessibility And First-Use Gate

Every action has provider/activity accessible name, keyboard focus, readable state/reason and non-color status. Use existing UI style/DPI policy; small glyphs have the usual hit target. At narrow widths collapse into a labeled `Capture` menu with the same entries/state rather than hide failures. Exercise supported scaling/high-contrast/keyboard routes and paths containing spaces/Unicode. Units and native target scope remain readable; serialized support identity is locale-independent.

`CHK-EC-UX` and `CHK-EC-ADOPT` retain clean first-use, failed setup, Busy, stale target, timeout/drain, open failure, automation and non-author recovery transcripts. Screenshots verify layout/state presentation only. Runtime capture, symbols, native validation, observer cost and Shipping proof require the separate dossier checks.
