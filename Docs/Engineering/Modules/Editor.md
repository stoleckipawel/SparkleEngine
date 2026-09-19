# Editor Engineering

**Status:** binding Editor integration standard

**Applies to:** `Engine/Editor`, ImGui, UI/render boundaries, editor background operations, capture UX, and interactive frontend workflows

The Editor owns user interaction and presentation, not mutable engine subsystem state. Import and cooking publication rules live in [Tools Engineering](Tools.md). Both preserve the canonical basis, units, spaces, and artifact representation defined by the [World Coordinate, Units, and Transform Contract](../../Architecture/Decisions/WorldCoordinateAndUnits.md).

## Editor Ownership

Editor main owns ImGui context and widget state, selection, immutable editor scene model, transactions/undo/redo, active-widget drafts, application/document lifetime, and application of narrow operation results.

Panels:

- read immutable models;
- identify world objects by stable ID;
- submit semantic commands;
- consume typed accepted/stale/rejected results;
- never retain `GameWorld*`, component pointers, mutable spans, registry views, renderer caches, live descriptors, or durable vector indices.

Continuous edits coalesce into bounded main-thread transactions with deterministic inverse commands or before/after values.

## UI And Render Boundary

- Editor owns view-mode presentation: labels, icons, menu grouping, shortcuts, selection interaction, and widget visibility. The selected value is the Renderer-owned `RenderViewMode`; Editor does not mirror it in a second enum or translate it through a preset object.
- The panel-owned `ViewportRenderRequest` is the single publication boundary. A mode change writes `ViewportRenderRequest::ViewMode` and advances that request's generation once; Renderer freezes the value into the corresponding `RenderView`. No mode value enters Renderer settings, Application command translation, or RHI.
- A mode must not be decomposed into overlapping target and flag authorities. If future independently selectable visibility controls are required, Editor may own their presentation and overrides, but they must have distinct Renderer consumers and must not reproduce the selected mode.
- A normal view-mode action never mutates process-global Renderer CVars. Renderer settings/CVars remain algorithm, scalability, or explicit developer policy; a genuinely global action uses the sequenced Renderer control boundary rather than direct Editor mutation.
- Renderer remains usable by Game/runtime code submitting the same `RenderViewMode` without loading Editor. The Editor menu is a presentation of that contract, not its owner.
- Copy ImGui draw data into packet-owned vertices, indices, clip rectangles, texture handles, and commands.
- Never send `ImDrawData*` or a live editor pointer to the render coordinator.
- Viewport requests/products use stable IDs or tokens plus explicit release or bounded retirement.
- Settings, preview, and capture use sequenced render commands.
- Editor owns the mutable rendering-settings interaction model and restart presentation. Application owns persistence and submits the resulting Renderer-owned value snapshot; Renderer must not contain the Editor controller or filesystem policy.
- Capture is bounded and nonblocking: the Application workflow retains the requested destination and exact Renderer ticket, polls only that ticket, and performs background encoding/publication after destination-free typed bytes and immutable product metadata arrive. `EditorApplication` invokes one `EditorViewportOutputCoordinator` and never names capture or renderer-feature coordinators, drains a global result queue, or dispatches payloads by feature. That coordinator is the narrow composition point for ordinary presentation capture and feature-owned viewport output. Generic image-buffer encoding, ordinary viewport writing, and verified atomic bundle publication are private Application implementors; feature folders retain only semantic output policy and schema.
- One private capture slot owns each admitted Renderer ticket through polling, delivery, or discard settlement. Capture consumers compose slots and semantic policy; they do not reproduce ticket IDs, optional readbacks, abandoned-ticket arrays, or polling loops.
- Public Editor UI exposes one bounded viewport-output intent for ordinary presentation capture and feature-owned output actions rather than one boolean/method/action type per feature. `EditorViewportOutputCoordinator` consumes ordinary capture directly and translates Reference artifact intent exactly once through `ReferencePathTracerArtifactActions`. The downstream coordinator consumes semantic work such as an artifact kind; UI action enums must not enter Renderer, RHI, codecs, or artifact mechanisms.
- Panel classes and mutable interaction controllers remain Editor-private. Public Editor headers expose the host facade and bounded semantic intents only; adding a panel is never a reason to grow another module's API.
- Close/cancel rejects late products before destroying their owner or model.
- Public Application/Editor host headers do not enumerate private panels, feature coordinators, codecs, or operation implementations. Keep those collaborators in private host state so the public host surface expresses lifecycle and user-facing capabilities only.
- Executable `main` functions call one non-overloaded Application launch entry. That boundary establishes process/thread state exactly once and constructs the interactive host; it does not dispatch feature-specific command modes.

## Background Operations

Use one private `EditorOperationRuntime` over a `SparkleTasks` document scope for shared cancellation and settlement lifetime only. Each feature coordinator owns its typed operation slot, concrete task body, result, and policy; the runtime must not include feature requests, writers, results, or a feature registry.

Reuse the private slot mechanism for task launch, settlement, occupied-state truth, and result storage. A coordinator may own semantic queued intent, but it must not mirror the slot with a second active/running boolean or overwrite a settled result before consuming it. Do not expose a callback-based generic job API, duplicate one task/execution scaffold per feature, or make an unrelated workflow edit a central operation switch.

- Inputs are immutable owned request values.
- Progress is bounded and coalesced.
- Results are immutable and applied on the owning thread.
- Search, preview, and reload use latest-generation-wins when that is the product policy.
- Close cancels, settles, rejects late results, then destroys state.
- Workers do not call ImGui or invoke UI callbacks.

## Intent-First Frontend Workflows

The frontend exposes the user's task; backend identities and tuning remain implementation detail unless they are necessary to make an expert decision. These rules also apply to an interactive tool frontend such as Launcher.

- Name primary actions after intent, such as `Compile Changed Shaders`, `Investigate GPU`, or `Validate And Export`, rather than cache, worker, package, query-pool, or backend operations.
- Prefer one recommended path with automatic capability detection, target selection, dependency closure, cache use, validation, publication, and safe lifetime handling. Do not ask the user to reproduce derivable backend policy.
- Use progressive disclosure: primary task and status first, contextual summary/next action second, searchable expert details and raw artifacts last.
- Keep one dominant action per state. Disable or remove impossible actions and state the prerequisite beside the action.
- Advanced overrides are explicit deviations from a named preset. They are typed, capability-validated, resettable, scoped, and visible in resulting configuration/evidence.
- Defaults may automate reversible, deterministic, local work. File export, external capture, destructive replacement, platform-limited behavior, or materially perturbing modes remain explicit and previewable.
- A failed operation leaves the previous accepted product active. Present one root cause with source/object identity and a useful next action; keep raw mechanics under details/copy/replay actions.
- Preserve selection and context across summary, detail, retry, and external handoff.
- Prefer the existing owner surface, command palette/console route, and operation service. A new panel, wizard, settings page, or provider callback requires a distinct durable user task.
- Keyboard navigation, search, readable focus, non-color state, and deterministic narrow-layout collapse are part of correctness.

Expert access means better inspection and an explicit override, not ownership of workers, cache files, native handles, graph scheduling, or lifetime mechanics.

## Editor Review Questions

- Does Editor main retain UI, selection, transaction, and model authority?
- Does Editor present and select the one Renderer `RenderViewMode` without defining a mirror enum, preset translation, global CVar route, or duplicate target/flag authority?
- Do panels use immutable models and semantic commands only?
- Does the normal workflow ask for user intent while deriving safe backend detail, with advanced deviations explicit and resettable?
- Are cross-thread UI/render products owned and late-result safe?
- Is background work scoped, bounded, cancellable, and settled before destruction?
- Does each capture workflow own its ticket, destination, encoding/publication policy, and late-result handling without adding a branch to the application host for every new consumer?
