# SparkleEngine Focus Cleanup Plan

This plan is about reducing architectural noise and context load. It is not a public README polish pass. The goal is to make the repository feel like one cohesive engine with a clear runtime, a clear cook pipeline, and fewer low-value surfaces competing for attention.

## Current Signal

The runtime engine structure is stronger than the repository first impression suggests. The existing module chain is already readable:

- `SparkleCore`
- `SparklePlatform`
- `SparkleRHI`
- `SparkleRenderer`
- `SparkleGameFramework`
- `SparkleEditor`
- `SparkleApplication`

The bigger source of bloat is around generated artifacts, debug/tool entry points, duplicated tool target patterns, and surfaces that are technically valid but not valuable enough to keep prominent.

## Cleanup Principles

- Keep real engine complexity when it proves ownership or capability.
- Remove or hide surfaces that mostly exist because they were useful during development.
- Prefer one obvious entry point per workflow.
- Keep runtime modules cooked-data-only.
- Keep source import and cooking behind tool-side ownership.
- Treat generated files as disposable output, not repository content.
- Add validation only when it protects a boundary we actually care about.

## Low-Value Removal Candidates

These are the first places to reduce context load. They are ordered from safest to most architectural.

### Generated And Local Output

Safe to remove from the working tree when not needed for active debugging:

- `build/`
- `logs/`
- `imgui.ini`
- `Projects/Showcase/build/`
- `Projects/Showcase/logs/`
- `Projects/Showcase/imgui.ini`

Current scan notes:

- `Projects/Showcase/logs/trace.json` was tracked generated output and has been removed from git tracking.
- `imgui.ini` and `Projects/Showcase/imgui.ini` are ignored local state.
- Root `build/`, root `logs/`, and project-local build/log directories exist as workspace clutter and should stay disposable.

Recommended action:

1. Keep `Projects/Showcase/logs/trace.json` untracked; it is generated output, not a committed fixture.
2. Keep `.gitignore` explicit about project-local generated output, not only root output.
3. Keep generated measurement logs out of commits unless they are explicitly attached to a performance investigation.
4. Use `Scripts/CleanWorkspace.bat` as the sanctioned cleanup entry point for disposable root and project-local generated output.

### Debug Tool Surfaces

Candidate surfaces to hide, fold, or remove from the main build story:

- `Tools/AssetConverter`
- `AssetCookerDll`
- `AssetCookerApi` C surface

Recommended stance:

- `AssetCooker` should be the primary cook entry point.
- `TextureCooker` and `ShaderCompiler` should remain standalone specialist tools.
- `AssetConverter` should become internal/debug-only, or be folded behind `AssetCooker` flows.
- `ConsoleDiagnostics` has been removed because it was an isolated debug utility with no clear workflow ownership.
- `AssetCookerDll` and the `AssetCookerApi` C surface have been removed; the CLI now talks directly to internal service ownership.

### Tool Aggregates

`SparkleCookTools` currently mixes public workflow targets and implementation/support targets. That makes the cook pipeline look wider than it is.

Recommended action:

1. Decide the public cook surface.
2. Make the aggregate target match that surface.
3. Move debug utilities behind opt-in targets or a separate internal aggregate.
4. Keep scripts and aggregate labels aligned with the public cook surface: `AssetCooker`, `TextureCooker`, and `ShaderCompiler`.

### Repeated Tool Boilerplate

The tool CMake files repeat setup for source collection, PCH, output directories, compile features, and link style.

Recommended action:

- Add a small CMake helper for tool targets only after the public tool surface is decided.
- Keep helper behavior boring and explicit: output directories, C++ standard, PCH, source groups, strict-warning hooks.
- Do not create a macro layer that hides target ownership.

### Duplicated Cook Policy

`MeshCooker` and `MaterialCooker` both build artifact identity/currentness policy near their write paths. That logic has low value when repeated per cooker.

Recommended action:

- Move generic artifact key/currentness helpers into `CookCommon`.
- Keep domain-specific texture decisions in `TextureCooker`.
- Merge mesh/material/scene cookers only if their boundaries are mostly artificial after the shared policy is extracted.

### Backend-Public Runtime Headers

`Engine/RHI/Public/D3D12` is acceptable for a D3D12-first engine, but it increases public surface area and makes the RHI look less abstract.

Recommended action:

- Leave it alone until after tool and artifact cleanup.
- Then audit which D3D12 headers are true backend contracts versus implementation detail.
- Move implementation-only headers under `Private` or a clearly backend-internal include surface.

## Keep Sharp

Do not remove these just to make the tree smaller:

- Runtime modules and their validation boundaries.
- Cooked asset formats and loaders.
- Shader cook/package validation.
- Texture cook pipeline stages.
- Showcase source assets and levels.
- Boundary validation scripts.
- Public scripts that are real user entry points.

These are high-signal architecture, even if they add file count.

## Execution Order

### Phase 1: Repository Hygiene

1. Keep tracked generated output out of the repo, starting with `Projects/Showcase/logs/trace.json`.
2. Keep `.gitignore` explicit about project-local generated output and generated measurement logs.
3. Clean local build/log/editor state from the working tree through `Scripts/CleanWorkspace.bat`.
4. Confirm `git status` contains only intentional source changes.

### Phase 2: Tool Surface Reduction

1. Keep `AssetCooker` as the public cook entry point and keep `AssetCookerDll` removed.
2. Decide whether `AssetConverter` remains a public executable.
3. Keep `ConsoleDiagnostics` removed unless a concrete diagnostic workflow justifies bringing back a focused replacement.
4. Update `Tools/CMakeLists.txt`, scripts, and validation rules to reflect those decisions.

### Phase 3: Shared Cook Policy

1. Extract repeated artifact identity/currentness helpers into `CookCommon`.
2. Update mesh/material/scene cookers to use the shared helpers.
3. Keep `TextureCooker` texture-specific decisions private to its pipeline.

### Phase 4: CMake Surface Cleanup

1. Keep the root entrypoint normalized as `CMakeLists.txt` and keep script/tooling references aligned to that name.
2. Keep validation target wiring extracted from the root file into `CMake/SparkleValidationTargets.cmake`.
3. Revisit a narrow tool-target helper only after tool ownership is fully settled.

### Phase 5: Runtime Public Surface Audit

1. Audit `Engine/RHI/Public/D3D12`.
2. Move backend implementation details private where practical.
3. Preserve runtime cooked-data and no-source-import validation.

## Validation Gates

Run these after relevant changes:

- CMake regeneration after CMake file changes.
- `sparkle_validation_check` after boundary or tool surface changes.
- `AssetCooker`, `TextureCooker`, and `ShaderCompiler` builds after tool changes.
- Runtime/editor module builds after engine CMake or public header changes.
- Targeted cook validation after changes to `AssetCooker`, `MeshCooker`, `MaterialCooker`, `SceneCooker`, or `TextureCooker`.

## Decision Log

- The repo does not need to become smaller by deleting engine capability.
- The repo should become sharper by removing generated output and low-value public surfaces.
- `AssetCooker` is the preferred public cook story.
- `TextureCooker` and `ShaderCompiler` are specialist tools worth keeping visible.
- `ConsoleDiagnostics` was removed as low-value debug surface.
- `AssetCookerDll` and the `AssetCookerApi` C surface were removed after confirming the tool CLI could call `AssetCookerService` directly.
- `Projects/Showcase/logs/trace.json` was removed from tracking as generated output.
- `AssetConverter` still needs justification to remain prominent.
- Validation target wiring now lives in `CMake/SparkleValidationTargets.cmake` instead of the root file.
- Root tracked filename is now normalized as `CMakeLists.txt`, and script/tooling references are aligned to that name.
- Documentation polish is secondary to architectural clarity and repository hygiene.