# SparkleEngine Agent Guidance

This is the canonical Codex guidance for SparkleEngine. Keep it compact and update it only when a rule prevents repeated mistakes or improves verification quality.

## Repository Map

- `Engine/`: C++20 engine modules: Core, Platform, RHI, Renderer, GameFramework, Editor, and Application.
- `Tools/`: developer tools, content import/cooking, shader compiler, asset conversion, and the Qt Sparkle Launcher.
- `Projects/`: runnable projects and sample content, currently including `Showcase`.
- `CMake/`: build profiles, dependency fetching, Qt discovery, artifact layout, and release assembly.
- `docs/`: planning, architecture notes, reviews, and implementation records.
- `.github/`: CI workflows.

## Generated Or Local-Only Folders

Treat these as generated or local state unless the user explicitly asks otherwise:

- `build/`
- `cmake-build-debug/`
- `artifacts/`
- `dist/`
- `logs/`

Do not place durable source, design decisions, or agent instructions in generated folders.

## Build And Validation

Sparkle uses CMake with custom build profiles:

- `DevelopmentEditor`
- `DevelopmentGame`
- `DebugEditor`
- `DebugGame`
- `ShippingEditor`
- `ShippingGame`

Default profile: `DevelopmentEditor`.

Candidate local commands:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config DevelopmentEditor --target SparkleLauncher -- /nologo /v:minimal /m:1
cmake --build build --config DevelopmentEditor --target ShaderCompiler -- /nologo /v:minimal /m:1
cmake --build build --config DevelopmentEditor --target clang_format_check -- /nologo /v:minimal
```

Choose the smallest meaningful validation:

- Docs-only: no build required; report that no runtime validation was run.
- Formatting-only C++ change: run `clang_format_check` when available.
- Launcher UI/core change: build `SparkleLauncher`.
- Shader compiler change: build `ShaderCompiler`; run cook/cache validation when scripts exist.
- Import/cook change: build the affected cooker/converter and run targeted sample cook when available.
- RHI/Renderer change: build the affected runtime/editor target and run smoke validation on D3D12/Vulkan when available.
- CMake/dependency/profile change: run a fresh configure plus targeted build.

## Code Style

- Follow `.clang-format`; do not introduce unrelated formatting churn.
- Respect `.clang-tidy`; warnings are configured as errors.
- Prefer existing local patterns, helper APIs, target names, naming, and module conventions.
- Keep edits narrow and behavior-focused.
- Avoid broad refactors unless explicitly requested.

## Module Boundaries

Respect the engine layer order:

```text
Core -> Platform -> RHI -> Renderer -> GameFramework -> Editor/Application
```

Tools may depend on engine surfaces, but runtime engine modules must not depend on tool internals. Keep source import, cooking, shader compilation, runtime loading, RHI, and renderer responsibilities separate.

## Graphics Review Posture

Work as if a senior graphics engineer may review every significant rendering change.

- For RHI, Renderer, shader compiler, and cook pipeline changes, identify the affected GPU/API contract before editing.
- State whether a change touches D3D12, Vulkan, shared RHI abstractions, shader reflection, cooked assets, runtime scene data, descriptors, synchronization, resource lifetime, or shader-visible layouts.
- Do not claim performance wins without measurement. If profiling tools are unavailable, provide a measurement plan and label expected bottlenecks as hypotheses.
- Preserve diagnostics, validation paths, logs, artifact paths, and reproduction steps when refactoring.
- Prefer precise technical language over hype.

## Launcher UI Work

For `Tools/Launcher/SparkleLauncher`, preserve the established Qt/CMake structure, shared visual style helpers, icon library, widget separation, and action/history model separation. Prefer small extraction and consistency passes over large rewrites.

Launcher UI work should be treated as product tooling for developers: clear workflows, reliable logs, recovery paths, and predictable build/cook/launch operations matter more than decorative UI churn.

## Dirty Worktree Discipline

The repo may contain user changes. Before editing a touched file, inspect it and work with the current state. Do not revert, overwrite, or reformat unrelated user changes. Never use destructive git operations unless the user explicitly asks.

## Final Response Expectations

When completing work, report:

- What changed.
- What validation was run.
- What validation could not be run and why.
- Any remaining risk, especially for RHI, Renderer, shader compiler, cook pipeline, CMake, or launcher workflow changes.
