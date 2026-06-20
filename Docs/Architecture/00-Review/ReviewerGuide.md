# SparkleEngine Reviewer Guide

## Purpose

This guide is the shortest path through SparkleEngine for a principal-level reviewer who needs to judge architecture quality, extension readiness, and validation discipline under time pressure. It points to the existing contracts, source roots, and commands that best reveal whether the engine is organized to support future low-level rendering, SDK, ray tracing, and neural-rendering work without becoming fragile.

## What To Review In 10 Minutes

Use this path if the goal is to decide whether the repository already shows recognizable production structure.

1. Read [README.md](../README.md) for module direction and ownership.
2. Read [A_PrincipalRoleRequirements.md](./A_PrincipalRoleRequirements.md) for the target bar.
3. Read [B_EngineArchitectureScorecard.md](./B_EngineArchitectureScorecard.md) for current strengths and gaps.
4. Inspect [CMake/ArchitectureBoundaryCheck.cmake](/C:/Users/stole/Documents/GitHub/SparkleEngine/CMake/ArchitectureBoundaryCheck.cmake:1) and [BoundaryRules.md](../01-Boundaries/BoundaryRules.md).
5. Check the main review contracts:
   - [RHIContract.md](../02-Contracts/RHIContract.md)
   - [RendererFrameGraph.md](../02-Contracts/RendererFrameGraph.md)
   - [ShaderPipeline.md](../02-Contracts/ShaderPipeline.md)
   - [RendererProviderContract.md](../02-Contracts/RendererProviderContract.md)
6. Read [ValidationMatrix.md](../03-Validation/ValidationMatrix.md) for the command surface and current coverage claims.

Quick judgment questions:

- Are module boundaries explicit and executable?
- Does `GameFramework` stay clear of `Renderer` and `RHI`?
- Is native API usage provider-scoped instead of leaking into general renderer policy?
- Are validation and smoke workflows visible enough for another engineer to reproduce?

## What To Review In 30 Minutes

Use this path if the goal is to assess whether the current architecture is a good base for future feature arrival.

1. Read [C_FoundationStagedPlan.md](./C_FoundationStagedPlan.md).
2. Read [ApplicationLifecycle.md](../02-Contracts/ApplicationLifecycle.md).
3. Read [RuntimeSceneData.md](../02-Contracts/RuntimeSceneData.md).
4. Read [PerformanceDiagnosticsPlan.md](../03-Validation/PerformanceDiagnosticsPlan.md).
5. Read [ADR/0001-renderer-native-api-provider-exceptions.md](../01-Boundaries/ADR/0001-renderer-native-api-provider-exceptions.md).
6. Compare the architecture docs with these module roots:
   - [Engine/RHI/CMakeLists.txt](/C:/Users/stole/Documents/GitHub/SparkleEngine/Engine/RHI/CMakeLists.txt:1)
   - [Engine/Renderer/CMakeLists.txt](/C:/Users/stole/Documents/GitHub/SparkleEngine/Engine/Renderer/CMakeLists.txt:1)
   - [Engine/GameFramework/CMakeLists.txt](/C:/Users/stole/Documents/GitHub/SparkleEngine/Engine/GameFramework/CMakeLists.txt:1)
   - [Engine/Application/CMakeLists.txt](/C:/Users/stole/Documents/GitHub/SparkleEngine/Engine/Application/CMakeLists.txt:1)
   - [Engine/Editor/CMakeLists.txt](/C:/Users/stole/Documents/GitHub/SparkleEngine/Engine/Editor/CMakeLists.txt:1)
   - [Tools/Shaders/ShaderCompiler/CMakeLists.txt](/C:/Users/stole/Documents/GitHub/SparkleEngine/Tools/Shaders/ShaderCompiler/CMakeLists.txt:1)

30-minute judgment questions:

- Do the docs describe ownership in the same direction as the source tree?
- Are renderer resource contracts named before new features are added?
- Are validation, diagnostics, and shader/tooling paths concrete enough to review?
- Are existing exceptions documented as temporary and scoped?

## Deep-Dive Path

Use this path when reviewing implementation quality rather than only structure.

1. Start with executable boundary enforcement:
   - [CMake/ArchitectureBoundaryCheck.cmake](/C:/Users/stole/Documents/GitHub/SparkleEngine/CMake/ArchitectureBoundaryCheck.cmake:1)
   - [BoundaryRules.md](../01-Boundaries/BoundaryRules.md)
   - [ADR/README.md](../01-Boundaries/ADR/README.md)
2. Review backend and lifetime ownership:
   - [RHIContract.md](../02-Contracts/RHIContract.md)
   - `Engine/RHI/Public`
   - `Engine/RHI/Private/D3D12`
   - `Engine/RHI/Private/Vulkan`
3. Review renderer orchestration and pass ownership:
   - [RendererFrameGraph.md](../02-Contracts/RendererFrameGraph.md)
   - `Engine/Renderer/Private/FrameGraph`
   - `Engine/Renderer/Private/FramePipeline`
   - `Engine/Renderer/Private/Passes`
   - `Engine/Renderer/Private/Diagnostics`
4. Review shader cook and ABI flow:
   - [ShaderPipeline.md](../02-Contracts/ShaderPipeline.md)
   - `Tools/Shaders/ShaderCompiler`
   - `Engine/Renderer/ShaderRegistrations`
5. Review provider isolation:
   - [RendererProviderContract.md](../02-Contracts/RendererProviderContract.md)
   - `Engine/Renderer/Private/Upscaling/NvidiaDlss`
6. Review runtime data ownership:
   - [RuntimeSceneData.md](../02-Contracts/RuntimeSceneData.md)
   - `Engine/GameFramework/Public`
   - `Engine/Renderer/Private/SceneData`
7. Review host lifecycle and workflow integration:
   - [ApplicationLifecycle.md](../02-Contracts/ApplicationLifecycle.md)
   - `Engine/Application`
   - `Engine/Editor`
   - `Tools/Launcher/SparkleLauncher`
8. Review evidence and observability:
   - [ValidationMatrix.md](../03-Validation/ValidationMatrix.md)
   - [PerformanceDiagnosticsPlan.md](../03-Validation/PerformanceDiagnosticsPlan.md)

## Build / Generate / Cook Validation Path

The fastest reviewer-facing workflow path is:

1. Architecture boundary check:
   - `cmake --build build --target architecture_boundary_check --config <profile>`
   - or `cmake -DSPARKLE_REPO_ROOT=<repo-root> -P CMake/ArchitectureBoundaryCheck.cmake`
2. Toolchain and dependency readiness:
   - `SparkleLauncher --run toolchain.check`
   - `SparkleLauncher --run workspace.sync-source-tiers`
3. Configure / generate:
   - `SparkleLauncher --run workspace.generate-build-files`
4. Build:
   - `SparkleLauncher --run workspace.build-all`
5. Cook:
   - `SparkleLauncher --run cook.project`
6. Optional format check:
   - `SparkleLauncher --format-mode check --run quality.format`

Primary evidence:

- launcher operation logs under launcher state
- build outputs under `build/private/...`
- cooked outputs under `artifacts/dev/projects/<Project>/cooked`

## Shader Inspection Path

Start with [ShaderPipeline.md](../02-Contracts/ShaderPipeline.md), then inspect the tooling surface directly.

Recommended commands:

- `ShaderCompiler list-backends`
- `ShaderCompiler list-targets`
- `ShaderCompiler inspect-shader <shader-id>`
- `ShaderCompiler inspect-package <path>`
- `ShaderCompiler cook [--package <package-id> | --shader-id <registered-shader-name>] [--target <name>] [--backend <name>] [--debug-artifacts <dir>]`

Review focus:

- source-to-package traceability
- reflection and parameter contract enforcement
- backend target declarations
- runtime registration ownership
- cache key and artifact stability

Source roots:

- `Tools/Shaders/ShaderCompiler`
- `Engine/Renderer/ShaderRegistrations`
- `Engine/RHI/Public`

## RHI / Backend Inspection Path

Start with [RHIContract.md](../02-Contracts/RHIContract.md).

Then inspect:

- `Engine/RHI/Public`
- `Engine/RHI/Private/D3D12`
- `Engine/RHI/Private/Vulkan`

Recommended validation commands:

- `SparkleLauncher --project <project-id> --launch-target runtime --smoke-test --smoke-backend d3d12 --run project.run`
- `SparkleLauncher --project <project-id> --launch-target runtime --smoke-test --smoke-backend vulkan --run project.run`

Review focus:

- device and queue ownership
- descriptor ownership
- allocator integration and memory budget visibility
- fence and submission lifetime
- backend-native interop containment
- parity claims versus what source actually proves

## Renderer / Frame Graph Inspection Path

Start with [RendererFrameGraph.md](../02-Contracts/RendererFrameGraph.md).

Then inspect:

- `Engine/Renderer/Public`
- `Engine/Renderer/Private/FrameGraph`
- `Engine/Renderer/Private/FramePipeline`
- `Engine/Renderer/Private/Passes`
- `Engine/Renderer/Private/Temporal`
- `Engine/Renderer/Private/RayTracing`
- `Engine/Renderer/Private/Diagnostics`

Review focus:

- frame lifecycle and pass lifecycle clarity
- transient versus persistent resource ownership
- history resource handling
- barrier and scheduling expectations
- temporal resource contracts such as color, depth, normals, motion vectors, exposure, history, jitter, camera matrices, and frame index

## SDK / Provider Inspection Path

Start with [RendererProviderContract.md](../02-Contracts/RendererProviderContract.md) and [ADR/0001-renderer-native-api-provider-exceptions.md](../01-Boundaries/ADR/0001-renderer-native-api-provider-exceptions.md).

Then inspect:

- `Engine/Renderer/Private/Upscaling/NvidiaDlss`
- `Engine/RHI/Public` interop and native handle surfaces
- `Tools/Launcher/SparkleLauncher/Private/Gui/Models/LauncherDependencyUiModel.cpp`

Review focus:

- whether provider integration is treated as one implementation, not as the renderer architecture
- capability-state transitions:
  - `unavailable`
  - `missing dependency`
  - `unsupported hardware`
  - `available`
  - `enabled`
  - `runtime failed`
- backend/provider-scoped native API permissions
- launcher reporting of missing dependencies versus unsupported hardware

## Diagnostics / Performance Inspection Path

Start with [PerformanceDiagnosticsPlan.md](../03-Validation/PerformanceDiagnosticsPlan.md) and [ValidationMatrix.md](../03-Validation/ValidationMatrix.md).

Then inspect:

- `Engine/RHI/Public/**/*Diagnostic*.h`
- `Engine/RHI/Public/**/*Memory*.h`
- `Engine/Renderer/Private/Diagnostics`
- `Engine/Editor/Private/**/*Profiler*.h`

Review focus:

- backend identity and adapter visibility
- memory budget, usage, and pressure visibility
- pass GPU timing and CPU timing visibility
- descriptor and upload pressure visibility
- separation between allocator-native truth and renderer-level summaries
- whether diagnostics produce reviewer-usable evidence instead of only internal hooks

## Known Limitations

- The architecture docs are now broad, but a few follow-up reviewer aids are still missing.
- `Renderer empty frame` validation is still planned rather than exposed as a dedicated stable command.
- Vulkan review coverage is conditional on local SDK and toolchain readiness.
- Pipeline cache statistics and some descriptor-pressure metrics are planned rather than fully surfaced.
- Backend availability reporting exists through launcher/toolchain flows, but not yet as a dedicated exported review report.

Planned follow-up docs:

- `Docs/Architecture/03-Validation/EditorDiagnostics.md` (planned)
- [LauncherWorkflowReadiness.md](../04-Workflows/LauncherWorkflowReadiness.md)
- `Docs/Architecture/03-Validation/BackendParityReview.md` (planned)

## What Is Intentionally Not Implemented Yet

- New FidelityFX, denoiser, frame-generation, or neural-rendering features are not being added in this documentation pass.
- The current effort does not widen renderer permissions to make provider integration easier.
- The current effort does not force parity claims where source evidence is incomplete.
- The current effort does not replace deeper runtime, RHI, or shader work with presentation-only docs.

## How To Judge Future Changes Against The Foundation

Use these checks when evaluating future work:

1. Boundary check first:
   - Does the change preserve the module direction documented in [README.md](../README.md) and enforced by [CMake/ArchitectureBoundaryCheck.cmake](/C:/Users/stole/Documents/GitHub/SparkleEngine/CMake/ArchitectureBoundaryCheck.cmake:1)?
2. Ownership clarity:
   - Is ownership obvious for lifetime, mutation, diagnostics, and failure handling?
3. Scope control:
   - Does a provider-specific need stay provider-scoped, or does it silently become a renderer-wide permission?
4. Validation evidence:
   - Is there a command, artifact, or smoke path that proves the feature still works?
5. Reviewer readability:
   - Can a senior external reviewer find the relevant contract and source root quickly?
6. Honest maturity:
   - Are `existing`, `partial`, `planned`, and `unknown` states still being used honestly?
7. Retirement discipline:
   - If a boundary exception is introduced, is it documented by ADR with a retirement path?

The architecture is improving when new features become easier to place, easier to validate, and harder to spread across the wrong modules.
