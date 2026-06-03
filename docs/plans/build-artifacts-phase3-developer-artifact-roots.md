# Phase 3 Developer Artifact Roots Handoff

Date: 2026-06-03

Scope:

- move daily runnable developer outputs to product-aware artifact roots
- keep direct CMake/MSBuild compatibility paths available during migration
- do not assemble `dist/` packages
- do not move cooked content
- do not run final build/package validation

Implemented Artifact Roots:

| Product role | Root |
| --- | --- |
| Launcher | `artifacts/dev/launcher/<Config>/` |
| Development tools | `artifacts/dev/tools/<Tool>/<Config>/` |
| Project editor | `artifacts/dev/projects/<Project>/editor/<Config>/` |
| Project runtime | `artifacts/dev/projects/<Project>/runtime/<Config>/` |
| Diagnostics | `artifacts/diagnostics/` |
| Symbols | `artifacts/symbols/<Owner>/<Config>/` |

Implemented CMake Contract:

- Added concrete dev artifact, diagnostics, and symbols roots to `CMake/SparkleArtifactContract.cmake`.
- Added `sparkle_configure_launcher_artifacts`.
- Added `sparkle_configure_development_tool_artifacts`.
- Added `sparkle_configure_project_artifacts`.
- Added `sparkle_declare_runtime_dll_owner`.
- Routed `SparkleLauncher`, `SparkleLauncherProbe`, and `SparkleLauncherCore` to launcher artifacts/symbols.
- Routed `AssetCooker`, `TextureCooker`, `ShaderCompiler`, and `AssetConverter` to development tool artifact roots.
- Routed `ShowcaseEditor` and `ShowcaseRuntime` to project editor/runtime artifact roots.
- Declared runtime DLL ownership for launcher, tool, editor, and runtime product executables when `SPARKLE_BUILD_SHARED=ON`.

Implemented Launcher Contract:

- Added launcher path helpers for artifact roots, diagnostics, symbols, launcher artifacts, development tool artifacts, and project target artifacts.
- Cook tool resolution now prefers `artifacts/dev/tools/<Tool>/<Config>/` and falls back to legacy `build/bin/<Config>/`.
- Launch executable resolution now prefers `artifacts/dev/projects/<Project>/<editor|runtime>/<Config>/` and falls back to legacy build output lookup.
- Launcher self-restart now targets `artifacts/dev/launcher/<Config>/`.
- Clean previews now point at product-aware artifact and symbol roots.
- Shader debug artifact defaults now use `artifacts/diagnostics/ShaderDebugArtifacts/<Project>/`.

Compatibility Notes:

- Engine module targets still keep their existing `build/bin` and `build/lib` output rules as direct build-system escape hatches in this phase.
- Product executables no longer depend on browsing those generic folders: shared runtime DLLs are copied to product artifact folders through declared owner/consumer relationships.
- Cooked content remains under the existing build cooked root until a later phase explicitly migrates cooked output policy.

Validation:

- Read-only searches confirmed launcher/tool/project product executable artifact helpers are wired.
- Remaining explicit `build/bin` output rules are engine module compatibility rules.
- Final build validation was not run.
- Final package validation was not run.
