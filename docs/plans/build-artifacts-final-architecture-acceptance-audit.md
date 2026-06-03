# Build Artifacts Final Architecture Acceptance Audit

Date: 2026-06-03

Purpose:

- verify the implemented repo shape against `docs/plans/build-artifacts-release-architecture-roadmap.md`
- remove final-state launcher/CMake compatibility fallbacks to legacy output roots
- document what is satisfied, how it is satisfied, and what still requires external toolchain availability

## Acceptance Summary

| Goal | Status | Evidence |
| --- | --- | --- |
| Keep generated build trees private and disposable | Satisfied | Root CMake defaults now write generic target outputs to `build/private/runtime/<Config>` and `build/private/lib/<Config>` instead of `build/bin` and `build/lib`. |
| Route developer products by product role and binary type | Satisfied | `sparkle_configure_launcher_artifacts`, `sparkle_configure_development_tool_artifacts`, `sparkle_configure_project_artifacts`, and `sparkle_configure_runtime_support_artifacts` route launcher, tools, project targets, runtime support, import/static libraries, and symbols through typed `artifacts/dev/...` and `artifacts/symbols/...` roots. |
| Remove launcher fallback to legacy executable roots | Satisfied | `ResolveSparkleToolPath` now returns artifact-root paths for launcher and development tools; it no longer checks `build/bin`. |
| Remove launcher fallback to legacy cooked roots | Satisfied | Launch readiness checks project and shared cooked artifact/package roots only; it no longer accepts `build/Cooked`. |
| Keep clean scopes precise and recoverable | Satisfied | Clean actions remove selected project cooked roots, all artifact cooked roots, build state, dependency cache, logs, or workspace state by explicit scope; legacy cooked cleanup is removed from primary maintenance workflows. |
| Separate host prerequisites from syncable source dependency groups | Satisfied | `Verify Host Environment` remains host-tool diagnostics; `Sync Source Dependencies` remains workspace/dependency configure flow. |
| Make launcher workflows outcome-first | Satisfied | Launcher catalog groups workflows into Start, Setup, Build, Cook, Run, Package, and Maintenance; package assembly is now a runnable workflow instead of a disabled placeholder. |
| Assemble release packages from artifacts into dist | Satisfied | `sparkle_release_assembly` consumes `artifacts/` and writes `dist/releases/<version>/sparkle-runtime-...` plus separate symbols package/archive. |
| Keep release publishing separate from package assembly | Satisfied | Package assembly reports reviewable packages; launcher text now states final validation/release sign-off remains separate from assembly. |
| Support launch-first package exploration | Satisfied by Phase 6 validation | Phase 6 smoke launched package-root `SparkleLauncher`, `ShowcaseEditor`, and `ShowcaseRuntime` from `dist/` before local rebuild/recook. |
| Preserve MSVC as recommended Windows path | Satisfied | Phase 6 validation used Visual Studio 2026/MSVC with Qt MSVC kit. |
| Preserve clang-cl support | Externally blocked | Presets/toolset support remains, but Phase 6 could not validate clang-cl because the toolchain was not installed or visible on the machine. |

## Fixes Applied During Audit

- Removed launch-readiness acceptance of `build/Cooked/<Project>` and `build/Cooked/Shared`.
- Removed maintenance clean target and process step for legacy cooked outputs.
- Removed development-tool and launcher executable fallback lookup through `build/bin`.
- Added `SPARKLE_DEV_RUNTIME_SUPPORT_ROOT` and `sparkle_configure_runtime_support_artifacts`.
- Added `SPARKLE_DEV_LIBRARY_ROOT` so import/static libraries are development artifacts, not symbols-package contents.
- Routed engine runtime support libraries through `artifacts/dev/runtime-support/<Target>/<Config>`.
- Replaced root generic `build/bin` and `build/lib` defaults with private build-tree defaults under `build/private/...`.
- Added `package.release` as a real launcher workflow backed by the `sparkle_release_assembly` CMake target.
- Updated stale Phase 3/Phase 4 handoff wording so it reflects the strict final architecture rather than temporary migration compatibility.
- Updated release assembly messaging so it no longer says validation was not run by a past phase.

## Legacy Remnant Scan Result

Final-state code should not contain launcher/CMake behavior that depends on:

- `build/bin`
- `build/Cooked`
- legacy executable fallback lookup
- legacy cooked fallback lookup
- disabled future-phase package assembly messaging

Remaining mentions of legacy concepts are acceptable only when they are:

- validation scripts that forbid old layouts
- roadmap statements describing forbidden assumptions
- unrelated runtime fallback terminology such as shader/debug/name fallback logic

## Final Validation Evidence

Phase 6 already produced an end-to-end validation report in `docs/plans/build-artifacts-phase6-final-validation-report.md`.

Validated:

- clean MSVC configure
- launcher build
- development tool builds
- Showcase editor/runtime builds
- shader, texture, and scene cook workflows
- package assembly into `dist/releases/0.0.0-phase6/...`
- package-root launcher startup smoke
- package-root editor/runtime startup smoke
- manifest and checksum verification

Not validated:

- clang-cl build smoke, because clang-cl was not installed or visible in the current environment
- publishing/signing/uploading, because package assembly is intentionally separate from release sign-off

## Audit Re-Verification

After removing the remaining compatibility fallbacks, the following verification was run successfully:

```text
"C:\Program Files\CMake\bin\cmake.exe" -S . -B build-phase6-msvc -DCMAKE_PREFIX_PATH=C:\Qt\6.11.1\msvc2022_64 "-DSPARKLE_PACKAGE_VERSION=0.0.0-phase6" -DSPARKLE_RELEASE_CHANNEL=dev
"C:\Program Files\CMake\bin\cmake.exe" --build build-phase6-msvc --config DevelopmentEditor --target SparkleLauncher AssetCooker TextureCooker ShaderCompiler ShowcaseEditor ShowcaseRuntime sparkle_release_assembly -- /m
```

Observed artifact/package outputs:

- `artifacts/dev/launcher/DevelopmentEditor/SparkleLauncher.exe`
- `artifacts/dev/tools/AssetCooker/DevelopmentEditor/AssetCooker.exe`
- `artifacts/dev/tools/TextureCooker/DevelopmentEditor/TextureCooker.exe`
- `artifacts/dev/tools/ShaderCompiler/DevelopmentEditor/ShaderCompiler.exe`
- `artifacts/dev/projects/Showcase/editor/DevelopmentEditor/ShowcaseEditor.exe`
- `artifacts/dev/projects/Showcase/runtime/DevelopmentEditor/ShowcaseRuntime.exe`
- `artifacts/dev/libraries/runtime-support/<Target>/DevelopmentEditor/*.lib`
- `artifacts/symbols/runtime-support/<Target>/DevelopmentEditor/*.pdb`
- `dist/releases/0.0.0-phase6/sparkle-runtime-0.0.0-phase6-dev-windows-x64`
- `dist/releases/0.0.0-phase6/sparkle-symbols-0.0.0-phase6-dev-windows-x64`

Verification notes:

- CMake configure completed and wrote build files successfully.
- Build completed with exit code 0.
- Existing third-party/developer warnings remain from Assimp/CMake FetchContent deprecations and Qt deploy environment detection; they did not block the architecture validation.

## Acceptance Decision

The build artifact architecture is functionally and structurally in the intended final shape for the MSVC Windows workflow:

- `build/` is private generated state
- `artifacts/` is the developer product surface
- `dist/` is the assembled package surface
- launcher workflows are user-outcome oriented
- optional rebuild/recook paths are explicit
- package launch-first exploration is supported
- old launcher compatibility fallbacks have been removed

The only remaining non-code blocker is installing or exposing clang-cl if clang validation is required for interview/release evidence.
