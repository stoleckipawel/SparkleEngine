# Phase 3 Developer Artifact Roots Handoff

Date: 2026-06-03

Scope:

- move daily runnable developer outputs to product-aware artifact roots
- route launcher-visible runnable products through product-aware artifact roots
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
- Cook tool resolution uses `artifacts/dev/tools/<Tool>/<Config>/` as the source-of-truth tool location.
- Launch executable resolution uses `artifacts/dev/projects/<Project>/<editor|runtime>/<Config>/` as the source-of-truth executable location.
- Launcher self-restart now targets `artifacts/dev/launcher/<Config>/`.
- Clean previews now point at product-aware artifact and symbol roots.
- Shader debug artifact defaults now use `artifacts/diagnostics/ShaderDebugArtifacts/<Project>/`.

Strict Final-State Notes:

- Engine module targets are private build products unless a product explicitly owns and stages their runtime files.
- Product executables no longer depend on browsing those generic folders: shared runtime DLLs are copied to product artifact folders through declared owner/consumer relationships.
- Cooked content is owned by project/shared artifact cooked roots after Phase 4.

Validation:

- Read-only searches confirmed launcher/tool/project product executable artifact helpers are wired.
- No launcher workflow depends on a generic `build/bin` executable lookup.
- Final build validation was not run.
- Final package validation was not run.
