# Editor and Tools

Status: binding domain integration standard

Applies to: Editor, ImGui, UI/render boundaries, cooking, import, compilation, capture, and background operations

Import and cooking must publish the canonical basis, units, spaces, and versioned artifacts defined by the [World Coordinate, Units, and Transform Contract](../../Architecture/WorldCoordinateAndUnits.md).

## Editor Ownership

Editor main owns ImGui context and widget state, selection, immutable editor scene model, transactions/undo/redo, active-widget drafts, application/document lifetime, and application of narrow operation results.

Panels:

- read immutable models;
- identify world objects by stable ID;
- submit semantic commands;
- consume typed accepted/stale/rejected results;
- never retain `GameWorld*`, component pointers, mutable spans, registry views, renderer caches, live descriptors, or durable vector indices.

Continuous edits coalesce into bounded main-thread transactions with deterministic inverse commands or before/after values.

## UI and Render Boundary

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

The frontend exposes the user's task; backend identities and tuning remain implementation detail unless they are necessary to make an expert decision.

- Name primary actions after intent, such as `Compile Changed Shaders`, `Investigate GPU`, or `Validate And Export`, rather than cache, worker, package, query-pool, or backend operations.
- Prefer one recommended path with automatic capability detection, target selection, dependency closure, cache use, validation, publication, and safe lifetime handling. Do not ask the user to reproduce derivable backend policy.
- Use progressive disclosure: primary task and status first, contextual summary/next action second, searchable expert details and raw artifacts last. Do not display a full internal record as the default screen merely because it is available.
- Keep one dominant action per state. Disable or remove impossible actions and state the prerequisite beside the action; do not accept an invalid combination and fail later when the frontend can prevent it.
- Advanced overrides are explicit deviations from a named preset. They are typed, capability-validated, resettable, scoped to project/platform or the current operation, and visible in the resulting configuration/evidence. Avoid independent booleans whose combinations are not all valid.
- Defaults may automate reversible, deterministic, local work. File export, external capture, destructive replacement, platform-limited behavior, or a mode that materially perturbs execution remains explicit and previewable.
- A failed operation leaves the previous accepted product active. Present one root cause with source/object identity and a useful next action; keep command lines, hashes, manifests, and raw logs under `Details`/copy/replay actions instead of leading with them.
- Preserve selection and context across summary, detail, retry, and external handoff. Opening another view must not make the user reconstruct the shader, frame, resource, workload, or configuration identity.
- Prefer the existing owner surface, command palette/console route, and operation service. A new panel, wizard, settings page, or provider callback requires a distinct durable user task that cannot fit the current surface.
- Keyboard navigation, search, readable focus, non-color state, and deterministic narrow-layout collapse are part of correctness, not polish deferred after the data path.

The UI may expose an expert escape hatch, but expert access means better inspection and an explicit override—not ownership of compiler workers, cache files, native handles, graph scheduling, or lifetime mechanics.

## Cooking, Import, and Publication

- Separate read, decode, transform, validate, and transactional publication by real ownership.
- Runtime loading remains cooked-only.
- Outputs are deterministic and transactionally replaced.
- Cancellation preserves the previous accepted artifact or world.
- File/process work is isolated from frame-critical capacity.
- Concurrency has a weighted memory budget for HDR textures, scenes, compiler sessions, and third-party workers.
- Do not add a second asset database, async loader family, or tool thread pool.

## Domain Review Questions

- Does Editor main retain UI, selection, transaction, and model authority?
- Do panels use immutable models and semantic commands only?
- Does the normal workflow ask for user intent while deriving safe backend detail, with advanced deviations explicit and resettable?
- Is the primary screen free of package/cache/hash/native-handle clutter, with one clear state and next action?
- Are cross-thread UI/render products owned and late-result safe?
- Is background work scoped, bounded, cancellable, and settled before destruction?
- Are imports and cooked artifacts deterministic and transactionally published?
- Are runtime and tool authorities kept separate?
