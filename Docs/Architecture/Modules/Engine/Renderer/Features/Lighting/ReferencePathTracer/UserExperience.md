# Reference Path Tracer User Experience Contract

**Status:** proposed development-product experience for `FCR-REN-08`; implementation and acceptance remain blocked until `PTD-00` ratifies the scope, mathematical claim, budgets, workflow, and evidence protocol

**Responsibility:** define how a developer, lighting engineer, or technical artist discovers, enters, navigates, observes, compares, resets, pauses, diagnoses, and secondarily exports or automates the Reference Path Tracer

**Authority boundary:** the [feature dossier](README.md) owns binary acceptance and failure verdicts, [Transport And Estimator](TransportAndEstimator.md) owns mathematical meaning, [Execution Architecture](ExecutionArchitecture.md) owns per-view session/state/data ownership, [Discovery](Discovery.md) owns ratification, and the [staged plan](Plan.md) owns implementation order and prompts

**Prepared:** 2026-09-09 against committed `master` revision `20c7bb11`; no UI, command, package, accessibility, or clean-machine workflow was implemented or exercised

**Naming reconciliation:** the 2026-09-09 working-tree clean break makes `ReferencePathTracer` the sole feature name; no UX capability or acceptance result is thereby implied.

**Priority reconciliation:** 2026-09-10 makes the live viewport comparison loop the first usable milestone and moves polished save/checkpoint/offscreen workflow behind it; this is a planning decision, not implementation evidence.

> [!IMPORTANT]
> The primary product is a first-class viewport view mode, not a separate render wizard. Selecting **Reference Path Tracer** must put the current view into a correct, bounded reference configuration and begin accumulation automatically. Export and noninteractive execution are secondary consumers of the same Renderer session semantics.

## Product Priority Order

Delivery and review follow this strict order:

1. **P0 — live viewport comparison loop:** the mode is immediately after Lit, starts automatically, continuously presents the newest completely committed prefix, remains navigable, visibly resets on camera/radiance changes, settles into uninterrupted accumulation when change stops, reports exact progress, and supports fast Lit-to-reference comparison.
2. **P1 — trustworthy Renderer/RHI reference semantics:** independent camera rays, one PBR-correct estimator, immutable Scene/View identity, deterministic samples, raw scene-linear accumulation, strict capability truth, and no stale-prefix mixing make the viewport result dependable.
3. **P2 — minimum acceptance observability:** bounded counters, state traces, raw readback, and provenance exist only as needed to falsify correctness and retain completion evidence.
4. **P3 — secondary artifact workflows:** polished save, checkpoint, offscreen, path-management, and automation conveniences follow the usable viewport slice. They reuse its session and never become a prerequisite for ordinary comparison.

P0 does not lower the mathematical bar: a responsive but physically wrong view is not usable. Conversely, artifact plumbing cannot be counted as progress toward the primary product while the live view is hidden, frozen during navigation, slow to reflect a new camera, unable to explain resets, or unsafe to compare with Lit. Final oracle authority still requires the evidence gates in the [feature dossier](README.md); this ordering controls implementation priority, not acceptance dilution.

## Product Pillar

A user opens the viewport View Mode menu, selects **Reference Path Tracer** immediately after **Lit**, and receives a progressively refined image with truthful sample progress. Any effective camera or radiance-affecting scene change invalidates the old prefix before it can mix with the new view. Switching back to Lit enables immediate comparison; returning to Reference Path Tracer resumes the exact prefix only when its complete identity is unchanged.

The normal route requires no IDE, developer console, CVar sequence, mandatory setup workspace, or manual `Validate`/`Start` ceremony. Correct defaults, capability validation, accumulation, reset, and preview presentation are consequences of selecting the view mode. A details surface exists for exact settings, diagnostics, pause/restart, and export, but is not a prerequisite for first use.

Four distinctions remain impossible to miss:

- **committed sample prefix versus target-SPP progress or estimated ETA**;
- **raw scene-linear accumulation versus the viewport display transform**;
- **complete current-view result versus partial prefix, checkpoint, export staging, failure, or accepted oracle evidence**;
- **viewport comparison session versus a durable exported artifact**.

The Renderer semantic supports both Editor and Game render views. The Editor exposes the selector in DevelopmentEditor. A non-Editor application may request the same semantic through its normal view configuration, but Shipping exposure remains excluded until release scope explicitly admits its product, dependency, and support obligations.

## Intended People And Jobs

| Persona | Primary job | Completion signal |
| --- | --- | --- |
| Lighting/PBR engineer | Switch Lit and Reference Path Tracer on one unchanged view to inspect a real-time lighting difference. | The reference prefix and reset state remain valid across the comparison switch, and both modes use the same canonical scene and camera identity. |
| Technical artist or content reviewer | Enter the mode without renderer setup, observe progress, move to another composition, and understand why accumulation restarted or cannot proceed. | The viewport always names the current state, exact prefix/target, and most recent invalidation or blocking semantic. |
| Evidence/release owner | Export a completed current-view result or reproduce it noninteractively with exact identity. | Raw EXR, manifest, uncertainty/counters, and comparison are linked to one input digest and exact prefix. |
| Support/graphics investigator | Reproduce a reset loop, unsupported scene, failed export, or backend problem. | One root cause, view/scene/setting identity, next action, replay manifest, and bounded support record are available. |

## View Mode Placement And One-Click Entry

The viewport menu order is stable:

1. **Lit**
2. **Reference Path Tracer**
3. the remaining diagnostic and wireframe modes under their existing organization

The Reference Path Tracer is a `RenderViewMode` semantic. It is not another value in a global real-time lighting-quality selector. On selection, the Renderer resolves one accepted mode preset:

- `SurfaceTransportReference` product;
- independent camera-ray primary visibility;
- fixed current render extent and accepted reconstruction filter;
- accepted automatic traversal/backend route, with requested and active values visible in details;
- stateless seed/sample/dimension stream and exact target SPP;
- raw scene-linear HDR accumulation with no ReSTIR, temporal reconstruction, denoiser, contribution clamp, exposure, tone map, gamut transform, encoder, or screenshot value in transport;
- separately applied viewport display transform for human inspection.

These values are mode-owned resolved semantics, not a batch of hidden mutations to persistent Lit settings. Leaving the mode restores the user's prior Lit configuration exactly. Expert overrides must be explicit, validated, identity-bearing, and resettable to the accepted mode preset.

If the mode is unsupported, the viewport retains its last valid presentation and shows **Reference Path Tracer unavailable** with the first failed capability/domain condition and one next action. It never silently falls back to Lit, ReSTIR, a GBuffer-seeded candidate, another backend, or black output while leaving the reference label selected.

## Primary Happy Path

```mermaid
flowchart LR
    Lit[Lit view] --> Select[Select Reference Path Tracer]
    Select --> Validate[Automatic capability and domain validation]
    Validate -->|supported| Accumulate[Accumulate exact sample prefix]
    Validate -->|blocked| Explain[Show blocker and next action]
    Accumulate --> Move[Camera or scene changes]
    Move --> Reset[Discard old prefix before next commit]
    Reset --> Accumulate
    Accumulate --> Complete[Complete target prefix]
    Complete --> Compare[Switch to Lit for comparison]
    Compare --> Return[Return to Reference Path Tracer]
    Return -->|identity unchanged| Complete
    Return -->|identity changed| Reset
    Complete -. optional evidence action .-> Save[Save raw result and manifest]
```

The minimum clean first-use flow is select, observe accumulation, navigate freely and see the displayed view follow with named resets, stop moving and watch the same view build samples automatically, then switch to Lit and back without losing a still-valid prefix. Saving is an optional evidence action outside that primary loop. Documentation may explain the physics and evidence model, but the happy path cannot require this dossier.

## View Session State And Dominant Action

One logical `ReferencePathTracerSession` belongs to one Renderer view identity. Names remain provisional until implementation review, but the visible semantics are fixed.

| State | User sees | Dominant action | Forbidden impression |
| --- | --- | --- | --- |
| `Inactive` | Normal non-reference view. | Select `Reference Path Tracer`. | Reference work is running in an unrelated view. |
| `Validating` | Current validation category and bounded elapsed time. | `Return to Lit`. | Sampling or a final digest already exists. |
| `Resetting` | A concrete reason such as `Reset — Camera rotated`, the discarded prefix, and new view identity. | Continue using the view. | Old and new samples are being blended. |
| `Accumulating` | Exact committed SPP, target, ratio, measured throughput, estimated ETA, active route, counters, and last reset reason. | Continue inspecting/navigating; secondary `Pause` and `Restart`. | A clean image or ETA means convergence/acceptance. |
| `Complete` | Persistent `Complete — N SPP` badge, identity, uncertainty status, counters, and comparison state. | Inspect or switch to Lit; secondary `Save Raw Result`. | Target SPP proves convergence or oracle authority. |
| `Suspended` | Retained prefix, identity, suspension reason, and whether GPU memory is retained. | Return to the mode or `Resume`. | Lit is still accumulating reference samples. |
| `Paused` | Exact retained prefix and resource/checkpoint status. | `Resume`. | Paused means complete. |
| `Checkpointing` *(secondary evidence state)* | Prefix being made durable and cancellation behavior. | `Cancel After Checkpoint` where safe. | A checkpoint is usable before verification. |
| `Exporting` *(secondary evidence state)* | Exact source prefix, raw/manifest staging progress, destination, and atomic-publication state. | `Cancel Export` where safe. | A visible staging path is a completed result. |
| `Unavailable` or `Failed` | One root cause, affected identity, safe state, retained valid prefix/artifact, cleanup, and recovery. | Context-specific repair/retry action. | A fallback or partial result is valid reference output. |

State color is supplementary. Text, icon/glyph, accessible label, and focus order communicate the same meaning. A transition becomes visible only when its underlying Renderer state is true; queue submission and GPU responsiveness are not progress.

## Viewport Progress Overlay

During validation, reset, accumulation, or failure, a compact non-modal overlay appears inside the viewport. It must not cover the view-mode selector or primary composition area and can collapse to a one-line status without hiding state.

The accumulating form contains:

```text
Reference Path Tracer
Accumulating 768 / 4096 SPP  [18.75%]
2.1 Msamples/s  ETA about 23 min
Last reset: Camera rotated
[Pause] [Restart] [Details]
```

Rules:

- `768 / 4096 SPP` is the exact completely committed per-pixel prefix; the bar is that ratio and never a convergence estimate. If the target is lowered below an already committed prefix, the bar stays complete and text reports both truths, for example `Complete — 1024 committed / target 512`; it never clamps or rewrites the prefix to fabricate `512 / 512`.
- ETA is visibly estimated and may show `Calculating`, `Unstable`, or `Unavailable`.
- Completion stops new sampling and replaces the bar with a persistent compact completion badge; the user does not have to infer completion from a hidden bar.
- Reset briefly names the cause and prior discarded prefix, then remains available in Details.
- During continuous camera or scene change, the overlay says why the prefix repeatedly restarts instead of suggesting that slow progress is a performance bug.
- Preview/display refresh and progress polling are bounded and coalesced. They do not synchronize every sample batch, change sample ordering, or delay camera feedback/cancellation beyond budget.

## Live Navigation Contract

Reference Path Tracer remains a live viewport mode while selected; it is not a still-image dialog that captures one camera and blocks interaction until completion.

- Editor and Game camera controls remain responsive while validation, reset, or accumulation is active. The last valid presentation may remain visible during a short transition, but it must be visibly stale/resetting and must not masquerade as the current camera result.
- Each effective camera change invalidates the prior measurement before commit. The Renderer may coalesce UI notifications and abandon superseded in-flight work, but it may never merge samples from two camera identities or weaken identity with a motion tolerance.
- During continuous movement, the viewport presents the newest available completely committed prefix for the newest accepted camera identity at the bounded preview cadence. A low-prefix noisy image is expected; a frozen old composition, black flicker used as a reset mechanism, or hidden stale image is not.
- When movement or rotation stops, accumulation continues automatically from ordinal zero for that final identity. The user never clicks `Restart`, `Validate`, or `Start` to make a moved view refine.
- Camera responsiveness takes scheduling priority over filling a now-stale batch, readback, checkpoint, export, or high-quality preview. This changes batch scheduling and presentation cadence only; it does not change sample identity, the estimator, or accepted prefix semantics.
- The overlay may coalesce a burst into `Camera moving — restarting for latest view`, then retain the final exact reset cause and discarded prefix in Details. Progress shown for the current identity remains exact.
- A continuously changing camera is not expected to converge. The useful contract is immediate visual orientation while moving and automatic progressive refinement once stable, with no stale-sample mixing.

## Accumulation Identity And Reset Contract

The session compares canonical semantic identity before committing every complete sample range. It never relies only on mouse/keyboard activity, approximate motion thresholds, a generic temporal-history-valid bit, or raw structure bytes. Path-tracer film jitter is generated from `MATH-09`; ordinary TAA jitter is excluded from the canonical camera identity.

| Change | Required response | Example visible reason |
| --- | --- | --- |
| Position, orientation, camera selection, pilot/eject, camera cut/teleport, projection type, FOV, aspect, near/far plane, orthographic height, admitted lens/aperture/focus/shutter/time field, crop, filter, or render extent changes | Atomically invalidate the old prefix before any new-view sample commits; start ordinal zero for the new digest. | `Camera moved`, `Projection changed`, `Viewport resized` |
| Geometry/instance transform/visibility/deformation, material/texture/alpha, light/emissive/environment, units, AS, shader/compiler, or any other contributing scene generation changes | Invalidate before mixing. If the producer cannot publish a trustworthy generation, block the affected domain rather than accumulating through it. | `Material changed: Brass`, `Scene geometry changed` |
| Product/domain, seed/replicate, sampler/dimension layout, transport-affecting setting, strict backend/frontend, or precision policy changes | Validate the new request, then reset to ordinal zero. | `Sampling configuration changed` |
| Target SPP increases | Continue the same exact stream from the committed prefix. | `Target raised to 4096 SPP` |
| Target SPP decreases to or below the committed prefix | Stop at the already committed prefix and report that exact count; never discard or pretend fewer samples were accumulated. | `Target met at 1024 committed SPP` |
| Exposure, tone mapper, gamut/output transform, false-color display derived from raw data, overlay layout, UI scale, or progress polling changes | Re-present the same raw prefix; no transport reset. | No reset; display lineage updates |
| Batch size, preview cadence, ETA model, wall-time budget extension, or scheduling changes | Preserve the sample stream and prefix unless a separately named resource failure forces safe suspension. | `Schedule updated` |
| Switch from Reference Path Tracer to Lit or another non-reference view mode | Suspend the view session after its last complete committed range. View-mode selection itself is not part of the transport digest. | `Suspended for Lit comparison` |
| Return to Reference Path Tracer | Resume only if the complete digest still matches. Otherwise discard the retained prefix and name the first changed field. | `Resumed 1024 SPP` or `Reset — Light changed` |
| Explicit `Restart` | Discard the current prefix, preserve settings, and restart at ordinal zero with an explicit reason/event. | `Manual restart` |

There is no epsilon for reference-camera movement: if canonical ray-generation input changes, the measurement changed. Canonicalization normalizes valid semantic values and rejects non-finite/invalid camera data; it does not hide small movement. A camera-input flag may improve the reason string but cannot be the invalidation authority.

## Editor And Non-Editor Cameras

Editor free-fly navigation, an Editor-piloted scene camera, and a Game/runtime camera are producers of one canonical `RenderViewInput.Camera` contract. The Reference Path Tracer observes the resolved View identity and camera fields after their normal owner has updated them. It does not poll Editor widgets, gameplay objects, or input devices independently.

- Editor translation, orbit, rotation, focus, bookmark recall, pilot/eject, projection edit, and scene-camera property edit all reset when they change the effective camera.
- Runtime camera animation, controller movement, camera cuts, teleports, active-camera replacement, projection/lens edits, and viewport resize follow the same rule.
- A producer's explicit cut/teleport/generation signal is retained as a precise reason, but exact canonical camera identity still catches unflagged changes.
- A frame counter, world tick, UI animation, or unchanged camera submission does not reset by itself.
- A continuously animated camera, material, light, transform, skin/morph state, or time-dependent shader continually creates new identities. The overlay explains the reset loop and recommends pausing simulation or choosing a frozen supported time. It never accumulates streaked/blended history and calls it reference.

The acceptance matrix exercises both `RenderViewKind::Editor` and `RenderViewKind::Game` through the same Renderer state path. Shipping reachability is a separate product gate, not a reason to fork camera semantics.

## Lit Comparison And Session Retention

Fast comparison is a first-class requirement:

- switching to Lit restores the user's prior Lit settings and suspends reference scheduling after a complete range;
- the view retains at most one bounded reference session/prefix under the accepted memory policy;
- returning revalidates and resumes without reset when scene, camera, extent, transport, shader, and sampler identity remain exact;
- changes made while viewing Lit invalidate the retained prefix through canonical generation observation, even though reference work is suspended;
- eviction under explicit memory pressure is visible as `Reference prefix released — memory policy`; returning starts clean and never implies a resume;
- a second viewport cannot silently steal the active accumulator. Initial single-active-session capacity reports the owning viewport and offers an explicit transfer/cancel action.

This retention is an interaction optimization only. It does not create a second accumulator authority, durable checkpoint, or evidence artifact.

## Settings And Automatic Preflight

Reference details are available from the progress overlay and rendering details surface:

| Group | Fields | UX rule |
| --- | --- | --- |
| Intent | Product/domain and exact authority label. | Accepted full-reference intent is first; finite diagnostic can never be mistaken for it. |
| Sampling | Exact target SPP, seed, replicate, sampler identity. | Target controls the requested prefix, not convergence. Seed/sampler changes reset; target changes follow the reset table. |
| View | View identity, camera, render extent/crop/filter, frozen time where admitted. | Read from the current canonical view; no second camera picker is required for the main path. |
| Execution | `Automatic (accepted routes only)`; strict backend/frontend under Expert. | Requested and active values are separate. Automatic never selects an unaccepted fallback. |
| Resources | GPU memory, wall-time, cancellation bounds, overlay/preview cadence; checkpoint policy under Evidence. | Predicted and active usage are visible; values are bounded. Camera response outranks secondary readback/export work. |
| Evidence/Output *(secondary, collapsed by default)* | Raw readback status, optional checkpoint, `Save When Complete`, destination, raw/AOV/manifest selection, optional display preview. | Ordinary viewport comparison requires none of these fields; prior completed output is preserved. |

Selecting the mode runs deterministic automatic preflight before sample zero. Supported defaults proceed without confirmation. A blocker names the first unsupported camera, scene object, material, light, feature-matrix row, capability, or capacity condition and offers a concrete next action. There is no `Render Anyway` for the raw reference product.

## Pause, Restart, Close, And Shutdown

- `Pause` stops assigning new ranges and retains a bounded in-memory exact prefix. The secondary `Checkpoint And Pause` action additionally writes and verifies a durable prefix before releasing promised resources.
- `Resume` revalidates the complete digest. It imports or retains nothing after the first mismatch.
- `Restart` is explicit and immediate after the current range settles; it cannot be triggered by a display-only setting.
- Closing a details panel does not affect the view session. Leaving the view mode suspends it under the comparison contract.
- Closing the viewport destroys or checkpoints the session according to the declared policy after bounded settlement; late GPU/readback/write results are rejected by generation.
- Application shutdown applies the selected bounded checkpoint-or-cancel policy and never publishes partial data as complete.

## Secondary Saving And Evidence Artifact Experience

Saving exists to retain evidence and exchange raw results; it is not the expected daily interaction and must not delay delivery or responsiveness of the live viewport comparison loop. The first usable milestone may omit polished saving, checkpoint, and offscreen UX. When these secondary surfaces land, they obey the same session identity:

- `Save Raw Result` is enabled for a complete current prefix and writes raw `beauty.exr`, named AOV/statistics EXRs, counters, hashes, and `manifest.json` last.
- `Save When Complete` registers an export intent against the current digest and target. An identity reset retargets only after explicit policy/confirmation; it never exports the discarded view under the new name.
- `Save Current Prefix` is an expert action. Its manifest says `PartialPrefix`, records the exact committed SPP, and cannot be discovered or labeled as a completed candidate reference.
- The optional viewport-looking image is labeled `Display Preview — not raw reference` and records exposure/tone/gamut/encoding plus the raw hash.
- Export consumes the same session accumulation through typed readback. It does not launch a second estimator or retrace an already complete matching view.
- Publication uses a unique staging sibling, hashes all artifacts, writes the completion manifest last, and atomically publishes on one volume. Failure preserves the in-memory prefix and all prior valid results.
- A save defaults to the exact current session render extent and raw precision. It never silently substitutes a thumbnail, viewport screenshot, UI-scaled backbuffer, preview texture, or lower default export resolution. Any explicit crop or extent change is identity-bearing and therefore starts a distinct measurement.

The Completed details lead with state, exact prefix/digest, authority label, uncertainty/counters, then `Open Folder`, `Copy Raw Path`, `Copy Manifest Path`, `Compare Raw...`, and `Copy Replay Command`. A screenshot, preview, checkpoint, staging directory, or failed partial output is never the default result.

## Noninteractive And Runtime Contract

Automation remains a secondary evidence surface and is not part of the first usable viewport milestone. One `ShowcaseEditor` operation may create an offscreen Game-kind RenderView and run the same Reference Path Tracer session, for example:

```text
ShowcaseEditor.exe --reference-path-tracer Saved/ReferencePathTracer/request.json
```

Stage 9 freezes the exact flag. The submission contains serializable Application intent only: project/level/camera locator, frozen time if supported, product/domain, extent/crop/filter, exact target SPP, seed/replicate, requested backend/frontend, budgets/checkpoint policy, and output destination. It never serializes Renderer handles, UI state, loaded scene data, descriptors, or a second estimator configuration.

Stable noninteractive terminal categories are `Completed`, `InvalidRequest`, `UnsupportedDomain`, `UnsupportedCapability`, `Cancelled`, `TimedOut`, `CapacityExceeded`, `DeviceLost`, `PublicationFailed`, `CheckpointRejected`, and `InternalFailure`. Equivalent viewport/offscreen intent resolves the same canonical Renderer digest and sample stream; invocation identity and wall-clock timing may differ.

A non-Editor interactive application requests `RenderViewMode::ReferencePathTracer` through its ordinary view-settings owner and presents progress through its own approved UI. A developer CVar may remain a diagnostic adapter, but is not the product contract.

## Error Message Contract

Every unavailable, reset, or failure presentation can expand to:

```text
What happened: one root-cause category in user language
Where: viewport, camera, scene object, material, light, setting, backend, checkpoint, or path identity
Why it matters: which accumulation, domain, or artifact invariant cannot be satisfied
What was preserved/discarded: exact prefix, checkpoint, in-flight range, prior result
Next action: one safe and specific recovery step
Details: stable category, reason code, digest, and copyable bounded support record
```

Examples:

- `Accumulation reset at 768 SPP: the viewport camera rotated. The discarded prefix will not be mixed with the new view; accumulation restarted at sample 0.`
- `Reference Path Tracer unavailable: material "GlassPane" uses transmission, outside Surface Transport Reference v0.1. Replace or exclude that content, or reopen PTD-00 scope. No samples were committed.`
- `Export failed: 18.4 GB is required and 7.1 GB is available at D:\Evidence. Choose another writable destination or free space. The completed viewport prefix and prior results are intact.`

Raw driver/compiler/RHI output stays in Details and is bounded and sanitized. A numeric native error, `Failed`, or `Reset` without the cause and consequence is insufficient.

## Accessibility, Input, Locale, And Scale

- The view-mode item, progress state, exact prefix, failure reason, and every action are keyboard reachable in deterministic order.
- State and severity are never color-only. Progress exposes a textual accessible value such as `768 of 4096 samples per pixel, accumulating`.
- Background sample updates do not steal focus. Unavailable/failure transitions focus the concise cause only when the user's initiating action requires it.
- Narrow viewports collapse ETA and secondary throughput before hiding state, prefix/target, reset reason, or primary action.
- UI scale and DPI changes do not reset transport unless the actual render extent changes.
- Numeric editing has explicit units, invariant serialization, locale-aware display, and round-trip-safe parsing. Manifests remain locale independent.
- Long names elide visually but stay accessible/copyable. Paths with spaces and non-ASCII characters are exercised end to end.
- Progress announcements are rate-limited; reset, failure, and completion remain promptly announced.

## Professional Defaults And Guardrails

- The default view-mode preset is accepted, full-reference, fixed-extent, raw-safe, and automatic-only-among-accepted-routes.
- It never enables firefly filtering, denoising, biased environment MIPs, path regularization, contribution clamps, adaptive stopping, or display output as raw.
- `Quick Diagnostic`, `Reference Candidate`, and evidence-owner presets expand to exact values. `Low/Medium/High/Ultra` is insufficient.
- Presets may change target SPP, crop, checkpoint cadence, and operational budgets; they may not silently change transport target, feature domain, material model, sampler, or bias policy.
- Resource limits fail or suspend explicitly before unsafe allocation. TDR-safe batching may change schedule, never the sample stream.
- Completed output is never overwritten in place. A retry/export gets a new invocation directory and lineage.

## Common Experience Failure Points

| Failure | Why it is unacceptable | Required design response |
| --- | --- | --- |
| Reference remains only a Lighting setting, CVar, or separate wizard | The main comparison route is hidden and disconnected from the viewport mental model. | One `RenderViewMode::ReferencePathTracer` item immediately after Lit; details/export are secondary. |
| Selecting the mode mutates persistent Lit settings | Returning to Lit is surprising and comparison is no longer controlled. | Resolve mode-internal reference semantics; restore untouched Lit state on exit. |
| User must click Validate and Start for the supported default | Routine comparison carries batch-tool ceremony. | Automatic preflight and start on view-mode selection; block only with an actionable reason. |
| Viewport freezes, displays the old composition, or turns black throughout camera movement | The feature behaves like an offline dialog instead of a view mode and makes composition search impractical. | Prioritize the newest camera identity, visibly mark transition state, present its newest committed prefix at bounded cadence, and refine automatically when motion stops. |
| Camera input is detected but effective camera changes are not, or vice versa | Editor/runtime producers can leave stale history or reset needlessly. | Canonical post-resolution camera identity is authority; producer cut/motion signals refine reasons only. |
| Small camera changes use an epsilon | Samples from different measurements mix. | Any canonical ray-generation change resets; no movement tolerance. |
| TAA jitter or frame index resets reference history | A stationary view never converges. | Path-tracer film samples own jitter; ordinary temporal jitter/frame count stay outside identity. |
| Animated content does not invalidate | Blurring or streaking is presented as reference. | Generation-complete dynamic-content invalidation or explicit unsupported/frozen-time rejection. |
| Switching to Lit destroys a valid prefix | The primary comparison loop is frustrating and expensive. | Bounded per-view suspension and exact-identity resume. |
| Switching modes always preserves a prefix | Scene/camera edits made in Lit can return stale output. | Revalidate the full digest on return and name the first mismatch. |
| One progress bar hides reset, sample commit, checkpoint, or export state | Stale or partial work can appear complete. | Exact state/prefix text plus separate checkpoint/export progress. |
| Progress percentage is called convergence | Fixed target SPP is mistaken for statistical proof. | Label it target-prefix progress; uncertainty/convergence remains separately evaluated. |
| Exposure or tone mapping resets transport | Comparison iteration wastes valid samples. | Presentation-only lineage refresh with no raw reset. |
| Display changes contaminate raw output | The oracle becomes presentation-dependent. | One-way raw-to-preview transform and raw-first save/compare actions. |
| A second viewport silently takes capacity | Work and memory disappear without an owner transition. | Explicit active-view capacity and transfer action. |
| Export launches another renderer | View and saved output can disagree. | Typed readback from the same exact session/prefix. |
| Shipping first run exposes the tool accidentally | Developer dependencies and unsupported workflow leak into the consumer product. | Explicit build/package/reachability gate; Renderer semantics do not imply product exposure. |

## UX Review And Acceptance Handoff

The implementation sequence has two non-interchangeable gates. The primary viewport usability gate passes before artifact polish can be used to claim product progress:

1. `CHK-RPT-15` records a clean first-use transcript for finding the mode after Lit, automatic validation/start, live progressive display, responsive camera translation/rotation, named reset, automatic post-motion refinement, exact progress/completion, Lit comparison, exact resume/reset on return, pause/restart, failure, and recovery;
2. both Editor and Game camera producers pass translation, rotation, camera selection, cut/teleport, projection/lens, resize, unchanged-frame, and continuous-motion matrices through one canonical View route;
3. `CHK-RPT-09` proves no stale sample survives any hard invalidation, the displayed composition follows the newest camera within its frozen responsiveness budget, and no presentation/scheduling-only change loses a valid prefix;
4. `CHK-RPT-16` proves one per-view Renderer session owns accumulation, no duplicate Lighting selector or estimator authority remains, and Shipping reachability matches accepted scope;
5. keyboard, focus, non-color, scale/DPI, narrow layout, and bounded-log cases pass; and
6. a first-time reviewer completes the primary loop without saving, source edits, IDE use, console commands, private explanation, or a setup wizard.

The complete workflow is ready for `FCR-REN-08` consideration only when the primary gate remains passing and:

7. `CHK-RPT-02` proves viewport, runtime, and noninteractive equivalent intents resolve to the same semantic digest and sample stream;
8. `CHK-RPT-13` exercises capability, capacity, timeout, cancellation, device, checkpoint, disk, and publication failures with bounded cleanup/recovery;
9. `CHK-RPT-10` proves every display/save/open/compare action keeps raw and presentation lineage separate; and
10. full-resolution raw save, locale, spaces, non-ASCII, read-only install, offscreen automation, support, and output discovery pass their declared evidence matrices.

This page defines the intended experience; the [feature dossier](README.md#acceptance-criteria) and signed `FCR-REN-08` report own the eventual verdict.

## Sources And Precedent

- Epic's official [Path Tracer documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/path-tracer-in-unreal-engine) is the direct interaction precedent: viewport View Mode selection, progressive accumulation for a still view, invalidation on camera/view/material/object change, target-SPP progress display, and runtime enablement. Its documented dynamic-animation invalidation gaps are a negative precedent, not behavior to reproduce.
- SparkleEngine [Editor Engineering](../../../../../../../Engineering/Modules/Editor.md) is the binding local authority for intent-first workflows, operation ownership, bounded background work, failure presentation, keyboard access, and lifecycle safety.
- The pinned [RTXPT reference controls](https://github.com/NVIDIA-RTX/RTXPT/blob/f08d1c739071e0faad0c7c274d861124c511abab/Rtxpt/SampleUI.cpp#L778-L866) and [Falcor PathTracer guide](https://github.com/NVIDIAGameWorks/Falcor/blob/eb540f6748774680ce0039aaf3ac9279266ec521/docs/usage/path-tracer.md#overview) are expert-control, reset, and progressive-rendering precedents, including negative lessons about defaults and label authority.
- AMD's pinned [Baikal workflow description](https://github.com/GPUOpen-LibrariesAndSDKs/RadeonProRender-Baikal/blob/2d5a5d0eb2092d75adf637bf6f381d7e9307e986/README.md) and [Radeon ProRender backend article](https://gpuopen.com/learn/radeon-prorender-2-02-10/) provide automation/data-generation and final-versus-preview precedents.
- The [OpenEXR Technical Introduction](https://openexr.com/en/latest/TechnicalIntroduction.html) informs raw channel, metadata, and lossless-format presentation; it does not define Sparkle's artifact or evidence authority.
- [Research](Research.md) owns the complete pinned implementation/source ledger, approved uses, and non-claims.
