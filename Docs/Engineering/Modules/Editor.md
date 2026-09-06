# Editor Engineering

Status: binding Editor integration standard

Applies to: `Engine/Editor`, ImGui, UI/render boundaries, editor background operations, capture UX, and interactive frontend workflows

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

- Copy ImGui draw data into packet-owned vertices, indices, clip rectangles, texture handles, and commands.
- Never send `ImDrawData*` or a live editor pointer to the render coordinator.
- Viewport requests/products use stable IDs or tokens plus explicit release or bounded retirement.
- Settings, preview, and capture use sequenced render commands.
- Capture is bounded and nonblocking: request, render copy/readback, GPU token, background encode/write, narrow result.
- Close/cancel rejects late products before destroying their owner or model.

## Background Operations

Use one private `EditorOperationService` over `SparkleTasks` scopes for owned workflows.

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
- Do panels use immutable models and semantic commands only?
- Does the normal workflow ask for user intent while deriving safe backend detail, with advanced deviations explicit and resettable?
- Are cross-thread UI/render products owned and late-result safe?
- Is background work scoped, bounded, cancellable, and settled before destruction?
