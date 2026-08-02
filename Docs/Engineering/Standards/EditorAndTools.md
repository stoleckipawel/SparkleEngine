# Editor and Tools

Status: binding domain integration standard

Applies to: Editor, ImGui, UI/render boundaries, cooking, import, compilation, capture, and background operations

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
- Are cross-thread UI/render products owned and late-result safe?
- Is background work scoped, bounded, cancellable, and settled before destruction?
- Are imports and cooked artifacts deterministic and transactionally published?
- Are runtime and tool authorities kept separate?

