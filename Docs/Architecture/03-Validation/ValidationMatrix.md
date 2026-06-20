# Validation Matrix

## Purpose

This document records how SparkleEngine currently demonstrates that architecture rules, build workflows, cook workflows, shader tooling, runtime/editor hosts, and selected rendering paths still work. It ties each validation surface to a command or workflow, expected artifacts, ownership, backend coverage, and current maturity.

## Non-goals

- This does not add new tests or automation.
- This does not invent commands that the repository does not expose.
- This does not claim backend parity or coverage that source does not prove.
- This is not a substitute for the deeper contracts in [RHIContract.md](../02-Contracts/RHIContract.md), [RendererFrameGraph.md](../02-Contracts/RendererFrameGraph.md), [ShaderPipeline.md](../02-Contracts/ShaderPipeline.md), or [ApplicationLifecycle.md](../02-Contracts/ApplicationLifecycle.md).

## Validation Categories

- Architecture boundary validation
- Workspace configure/build workflow validation
- Cook workflow validation
- Formatting and code health validation
- RHI/backend startup and presentation validation
- Shader compile/reflection/package validation
- Runtime/editor host validation
- Dependency and backend availability validation

## Matrix

| Validation name | Purpose | Command or planned command | Backend coverage | Artifact output | Failure owner | Current status |
| --- | --- | --- | --- | --- | --- | --- |
| Architecture boundary check | Prevent Renderer/RHI/Application validation boundary regressions and counted exception drift. | `cmake --build build --target architecture_boundary_check --config <profile>` or `cmake -DSPARKLE_REPO_ROOT=<repo-root> -P CMake/ArchitectureBoundaryCheck.cmake` | Source-only, backend-agnostic rule scan | CMake stdout/stderr; build failure on violation | CMake architecture guardrails; module owners for violating files | existing |
| Configure / Generate Build Files | Validate toolchain detection, generator selection, configure prerequisites, and workspace feature set. | `SparkleLauncher --run workspace.generate-build-files` | Host/toolchain only; feature flags can include Vulkan/Streamline gating | Launcher operation log at `LauncherState/Logs/workspace.generate-build-files/Configure.txt` | Launcher build workflow and toolchain detection | existing |
| Build All | Validate end-to-end local rebuild of launcher, selected editor/runtime targets, and enabled cook tools. | `SparkleLauncher --run workspace.build-all` | Backend-independent build orchestration; compiled targets may include D3D12/Vulkan code paths | Launcher logs under `LauncherState/Logs/workspace.build-all/`; build outputs under `build/private/...` and `artifacts/dev/...` | Launcher build workflow; owning module/target on compile failure | existing |
| Cook All | Validate combined shader, texture, and scene/mesh cook readiness for a selected project. | `SparkleLauncher --run cook.project` | Shader cook can target DXIL and SPIR-V; asset/textures are content-pipeline coverage, not backend parity | Launcher logs under `LauncherState/Logs/cook.project/`; cooked outputs under `artifacts/dev/projects/<Project>/cooked` | Launcher cook workflow; ShaderCompiler / AssetCooker / TextureCooker owners | existing |
| Format Check | Validate source formatting or apply clang-format consistently across Engine and Projects source trees. | `SparkleLauncher --format-mode check --run quality.format` or CMake target `clang_format_check` | Backend-agnostic | Launcher logs under `LauncherState/Logs/quality.format/`; clang-format stdout/stderr | Launcher maintenance workflow; code owners for formatting violations | existing |
| RHI D3D12 startup / teardown | Prove D3D12 backend can initialize, run a smoke frame sequence, and shut down cleanly through the host path. | `SparkleLauncher --project <project-id> --launch-target runtime --smoke-test --smoke-backend d3d12 --run project.run` | D3D12 | Launch log plus optional smoke capture artifacts | Launch/smoke workflow plus RHI D3D12 backend | partial |
| RHI Vulkan startup / teardown | Prove Vulkan backend can initialize, run a smoke frame sequence, and shut down cleanly when Vulkan is available. | `SparkleLauncher --project <project-id> --launch-target runtime --smoke-test --smoke-backend vulkan --run project.run` | Vulkan only when backend and SDK are available | Launch log plus optional smoke capture artifacts | Launch/smoke workflow plus RHI Vulkan backend | partial |
| Swapchain / presentation smoke | Validate present-path viability, viewport products, and host presentation flow. | `SparkleLauncher --project <project-id> --launch-target editor --smoke-test --smoke-capture <path> --run project.run` | D3D12 and Vulkan through selected smoke backend; editor path is best evidenced today | Launch log, scene-color capture, capture `.json`, capture `.timing.csv` | Launch/smoke workflow, Renderer presentation path, RHI presentation service | partial |
| Shader compile | Validate offline shader compilation, target selection, cache behavior, and cook summary emission. | `ShaderCompiler cook [--package <package-id> | --shader-id <registered-shader-name>] [--target <name>] [--backend <name>] [--debug-artifacts <dir>]` or `SparkleLauncher --run cook.shaders` | DXIL and SPIR-V where supported by selected backend/toolchain | ShaderCompiler console output, cache directory, recook signal, optional debug artifacts | ShaderCompiler tooling | existing |
| Shader reflection verification | Validate reflected shader bindings against parameter struct contracts during cook. | `ShaderCompiler cook ...` or `SparkleLauncher --run cook.shaders` | DXIL and SPIR-V reflection paths; exact target coverage depends on cook selection | Cook log; optional debug artifact `reflection.json` per bundle | ShaderCompiler verification pipeline | existing |
| Shader package inspection | Inspect cooked package layout, binary metadata, reflection counts, backend name, and codegen target. | `ShaderCompiler inspect-package <path>` and `ShaderCompiler inspect-shader <shader-id>` | Depends on cooked package contents; not a runtime backend test by itself | CLI stdout; package file remains unchanged | ShaderCompiler tooling | existing |
| Renderer empty frame | Demonstrate renderer can execute a minimal host frame and produce smoke diagnostics without requiring a feature scene. | Planned command | Planned D3D12 and Vulkan coverage | Planned diagnostics artifact and/or capture | Renderer + Application host | planned |
| Runtime project load | Validate startup level resolution, scene asset load, level switching, and runtime host loop. | `SparkleLauncher --project <project-id> --launch-target runtime --smoke-test --run project.run` | Selected smoke backend | Launch log; optional smoke artifacts; runtime exit code | Application runtime host + GameFramework level/scene ownership | partial |
| Editor startup | Validate embedded runtime host, UI bring-up, viewport presentation, diagnostics wiring, and editor shutdown. | `SparkleLauncher --project <project-id> --launch-target editor --run project.run` | Selected backend through editor runtime host | Launch log; optional smoke artifacts when smoke mode is enabled | Application editor host + Editor module | existing |
| Dependency sync / source availability | Validate enabled source dependency caches, tool bundle presence, and configure retry/recovery behavior. | `SparkleLauncher --run workspace.sync-source-tiers` and `SparkleLauncher --run toolchain.check` | Host/tooling only | Launcher logs under `LauncherState/Logs/workspace.sync-source-tiers/` and `.../toolchain.check/` | Launcher dependency + toolchain workflow | existing |
| Backend availability reporting | Validate that launcher/tooling surfaces D3D12/Vulkan/Streamline-related readiness and host capability state. | `SparkleLauncher --dry-run toolchain.check` and `SparkleLauncher --dry-run workspace.generate-build-files` | Host/tooling visibility for D3D12 default path, Vulkan SDK readiness, NVIDIA gating | Dry-run text plus latest launcher log; UI summaries when run through launcher GUI | Launcher toolchain detection + host graphics capability reporting | partial |

## Status Meanings

- `existing`: command/workflow exists and is source-backed in the repo today.
- `partial`: validation behavior exists, but coverage, artifacts, or contract surface is incomplete.
- `planned`: the architecture clearly wants this validation, but the repo does not yet expose a stable command.
- `unknown`: source was not sufficient to prove the validation surface.

## Artifact Directory Policy

Validation artifacts should be easy to triage and should separate durable product outputs from local workflow evidence.

- Launcher workflow logs:
  - `GetLauncherOperationLogPath(...)`
  - default root resolves under launcher state, for example `%LOCALAPPDATA%/SparkleEngine/LauncherState/<repo-key>/Logs/...` on Windows
- Build outputs:
  - `build/private/runtime/<profile>`
  - `build/private/lib/<profile>`
- Project and tool artifacts:
  - `artifacts/dev/...`
- Cooked project outputs:
  - workspace/dev path: `artifacts/dev/projects/<Project>/cooked`
  - package-runtime path when applicable: `Projects/<Project>/Cooked`
- Shader cache and recook publication:
  - `Paths::ShaderCacheRoot()`
  - `Paths::ShaderRecookSignal(...)`
- Shader debug artifacts:
  - `Paths::ShaderDebugArtifactRoot()`
  - bundle-level artifacts can include `reflection.json`
- Smoke capture artifacts:
  - caller-selected image path via `--smoke-capture` or `SPARKLE_SMOKE_SCENE_COLOR_CAPTURE`
  - sibling metadata: `<capture>.json`
  - sibling timings: `<capture>.timing.csv`
- Trace output:
  - default `logs/trace.json`
- Review/package outputs:
  - `dist/releases/<version>`

Policy rules:

1. Logs should stay under launcher-state directories, not mixed into cooked outputs.
2. Cooked outputs should stay under project-scoped cooked roots.
3. Debug and inspection artifacts should be reproducible from commands, not handwritten.
4. Smoke captures should always allow sidecar metadata and timing output.
5. Review packages should stay separate from local developer validation noise.

## Failure Triage Policy

Use the first failing validation surface that is closest to the cause.

1. Architecture boundary failures:
   - owner: module author of the violating include/native dependency
   - first stop: `CMake/ArchitectureBoundaryCheck.cmake` and [BoundaryRules.md](../01-Boundaries/BoundaryRules.md)
2. Configure/build workflow failures:
   - owner: launcher build workflow and relevant toolchain/dependency owner
   - first stop: launcher operation log and `BuildWorkspaceExecutor.cpp`
3. Dependency or SDK failures:
   - owner: launcher toolchain detection and source dependency policy
   - first stop: `toolchain.check`, `workspace.sync-source-tiers`, and configure failure summaries
4. Shader cook/reflection/package failures:
   - owner: ShaderCompiler tooling
   - first stop: ShaderCompiler stdout/log, cache directory, debug artifacts
5. Runtime/editor smoke failures:
   - owner: Application host, Renderer, RHI backend, or GameFramework depending on failing evidence
   - first stop: launch log, smoke capture `.json`, `.timing.csv`, and renderer smoke diagnostics
6. Formatting failures:
   - owner: code author for touched files
   - first stop: `quality.format` log or `clang_format_check`

Escalation rule:

- If a smoke failure only reproduces on one backend, route it first to that backend owner before broad renderer refactor work starts.

## New Validation Checklist

1. Decide whether the new validation belongs to architecture, launcher/build, cook, shader, runtime/editor host, renderer, or RHI.
2. Expose a stable command, launcher operation id, or explicit planned command entry.
3. Record backend coverage honestly:
   - D3D12 only
   - Vulkan only
   - both
   - backend-agnostic
4. Define artifact outputs before adding more assertions.
5. Identify a single primary failure owner even if multiple modules participate.
6. Add the validation to this matrix with one of:
   - `existing`
   - `partial`
   - `planned`
   - `unknown`
7. If the validation introduces a new exception to architecture boundaries, add or update an ADR instead of silently widening rules.
8. If the validation depends on environment variables, document the controlling variables and output paths.
9. Prefer launcher-visible workflow ids when the validation is meant for reviewer use.
10. Do not mark backend parity as complete until both command surface and artifact evidence exist.

## Known Gaps

- There is no single dedicated `renderer empty frame` command yet, even though smoke infrastructure is close to supporting it.
- RHI startup/teardown validation is currently mediated through application/launch smoke rather than a backend-focused conformance harness.
- Vulkan coverage is real but still conditional on SDK/toolchain availability and does not yet have a reviewer-facing parity dashboard.
- Backend availability reporting exists in launcher/toolchain status, but it is not yet a dedicated contract report with artifact snapshots.
- Format validation is well-surfaced in launcher and CMake, but there is no unified report artifact beyond logs.
- Shader verification is strong inside the cook pipeline, but golden/regression corpus coverage is not yet formalized here.
- The architecture boundary check exists as a CMake target, but it is not yet promoted as a first-class launcher validation action.
