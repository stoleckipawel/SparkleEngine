# SparkleEngine Architecture Docs

These docs make SparkleEngine reviewable as a low-level, hardware-aware rendering engine foundation. Start here when you need to understand module direction, ownership, validation, and the staged path toward future renderer, SDK, ray tracing, and neural-rendering work.

## Directory Map

```text
Docs/Architecture
|-- README.md
|-- 00-Review
|   |-- ReviewerGuide.md
|   |-- A_PrincipalRoleRequirements.md
|   |-- B_EngineArchitectureScorecard.md
|   |-- C_FoundationStagedPlan.md
|   |-- D_ImplementationPrompts.md
|   `-- PrincipalRenderingReadiness.md
|-- 01-Boundaries
|   |-- BoundaryRules.md
|   `-- ADR
|       |-- README.md
|       `-- 0001-renderer-native-api-provider-exceptions.md
|-- 02-Contracts
|   |-- RHIContract.md
|   |-- RendererFrameGraph.md
|   |-- RendererProviderContract.md
|   |-- ShaderPipeline.md
|   |-- RuntimeSceneData.md
|   `-- ApplicationLifecycle.md
|-- 03-Validation
|   |-- ValidationMatrix.md
|   `-- PerformanceDiagnosticsPlan.md
|-- 04-Workflows
|   `-- LauncherWorkflowReadiness.md
`-- 05-Implementation
    `-- PortfolioReadinessImplementationPlan.md
```

## Where To Start

For a time-boxed external review:

1. [ReviewerGuide.md](./00-Review/ReviewerGuide.md)
2. [A_PrincipalRoleRequirements.md](./00-Review/A_PrincipalRoleRequirements.md)
3. [B_EngineArchitectureScorecard.md](./00-Review/B_EngineArchitectureScorecard.md)
4. [C_FoundationStagedPlan.md](./00-Review/C_FoundationStagedPlan.md)
5. [ValidationMatrix.md](./03-Validation/ValidationMatrix.md)

For implementation planning:

1. [D_ImplementationPrompts.md](./00-Review/D_ImplementationPrompts.md)
2. [BoundaryRules.md](./01-Boundaries/BoundaryRules.md)
3. [RHIContract.md](./02-Contracts/RHIContract.md)
4. [RendererFrameGraph.md](./02-Contracts/RendererFrameGraph.md)
5. [ShaderPipeline.md](./02-Contracts/ShaderPipeline.md)

For launcher/build/cook workflow readiness:

1. [LauncherWorkflowReadiness.md](./04-Workflows/LauncherWorkflowReadiness.md)
2. [ValidationMatrix.md](./03-Validation/ValidationMatrix.md)
3. [PerformanceDiagnosticsPlan.md](./03-Validation/PerformanceDiagnosticsPlan.md)

For code and asset execution planning:

1. [PortfolioReadinessImplementationPlan.md](./05-Implementation/PortfolioReadinessImplementationPlan.md)
2. [ValidationMatrix.md](./03-Validation/ValidationMatrix.md)
3. [PerformanceDiagnosticsPlan.md](./03-Validation/PerformanceDiagnosticsPlan.md)

## Folder Purposes

`00-Review`

- Reviewer entrypoints, role requirements, scoring, staged plan, and implementation prompts.
- This is the best starting point when evaluating the engine under time pressure.

`01-Boundaries`

- Executable architecture boundary rules and ADRs for intentional exceptions.
- This is where dependency direction and native API escape hatches are documented.

`02-Contracts`

- Module and subsystem contracts for RHI, Renderer, providers, shaders, runtime scene data, and application lifecycle.
- This is where future feature work should look before adding code.

`03-Validation`

- Validation command matrix, artifact expectations, failure ownership, performance, memory, and diagnostics planning.
- This is where "the engine still works" should become concrete evidence.

`04-Workflows`

- Launcher workflow, dependency, toolchain, generator, backend, and provider readiness policy.
- This is where reviewer-facing generate/build/cook/sync behavior is explained.

`05-Implementation`

- Staged code and asset implementation prompts derived from the review, contract, validation, diagnostics, and workflow docs.
- This is where the repository moves from documented readiness goals to executable portfolio evidence.

## Dependency Direction

Dependency direction is intentionally downward:

- `Core` provides shared utilities, diagnostics, math, time, events, and base runtime support.
- `Platform` provides platform-facing runtime support used by engine modules.
- `RHI` sits below the renderer and owns backend-facing graphics API abstraction and backend-private implementations.
- `GameFramework` owns runtime scene and cooked asset data and must not depend on `Renderer` or `RHI`.
- `Renderer` consumes `RHI` plus runtime scene/material data from `GameFramework`.
- `Application` hosts runtime/editor lifecycle above `Renderer` and runtime world code.
- `Editor` inspects and drives runtime rendering through tool-facing UI and diagnostics.
- `Launcher` orchestrates sync, generate, build, cook, clean, launch, and dependency/tool workflows.
- `ShaderCompiler`, `ShaderContracts`, and `AssetCooker` are tooling surfaces that support the runtime architecture without redefining it.

Executable boundary checks currently enforce key parts of this direction in [CMake/ArchitectureBoundaryCheck.cmake](/C:/Users/stole/Documents/GitHub/SparkleEngine/CMake/ArchitectureBoundaryCheck.cmake:1).

## Module Map

```text
Core        Platform
  |            |
  +-----+------+-----------------------------+
        |                                    |
        v                                    v
 GameFramework                         SparkleLauncher
        |                              ShaderCompiler
        |                              ShaderContracts
        |                              AssetCooker
        v
       RHI
        ^
        |
    Renderer
        ^
        |
   Application
        ^
        |
      Editor
```

Reading notes:

- `Renderer` depends on `RHI` and runtime scene data, but renderer code must not become a place where native D3D12/Vulkan details casually spread.
- `GameFramework` is a runtime data layer, not a rendering backend layer.
- `Editor` is allowed to inspect `Renderer` and `RHI` contracts through tool-facing surfaces.
- `Launcher` is a host tool and workflow orchestrator, not part of runtime render architecture.
- Tooling modules support shader cook/build/asset workflows and reviewer validation paths.

## Current Doc Set

Review:

- [ReviewerGuide.md](./00-Review/ReviewerGuide.md)
- [A_PrincipalRoleRequirements.md](./00-Review/A_PrincipalRoleRequirements.md)
- [B_EngineArchitectureScorecard.md](./00-Review/B_EngineArchitectureScorecard.md)
- [C_FoundationStagedPlan.md](./00-Review/C_FoundationStagedPlan.md)
- [D_ImplementationPrompts.md](./00-Review/D_ImplementationPrompts.md)
- [PrincipalRenderingReadiness.md](./00-Review/PrincipalRenderingReadiness.md)

Boundaries:

- [BoundaryRules.md](./01-Boundaries/BoundaryRules.md)
- [ADR/README.md](./01-Boundaries/ADR/README.md)
- [ADR/0001-renderer-native-api-provider-exceptions.md](./01-Boundaries/ADR/0001-renderer-native-api-provider-exceptions.md)

Contracts:

- [RHIContract.md](./02-Contracts/RHIContract.md)
- [RendererFrameGraph.md](./02-Contracts/RendererFrameGraph.md)
- [RendererProviderContract.md](./02-Contracts/RendererProviderContract.md)
- [ShaderPipeline.md](./02-Contracts/ShaderPipeline.md)
- [RuntimeSceneData.md](./02-Contracts/RuntimeSceneData.md)
- [ApplicationLifecycle.md](./02-Contracts/ApplicationLifecycle.md)

Validation and diagnostics:

- [ValidationMatrix.md](./03-Validation/ValidationMatrix.md)
- [PerformanceDiagnosticsPlan.md](./03-Validation/PerformanceDiagnosticsPlan.md)

Workflows:

- [LauncherWorkflowReadiness.md](./04-Workflows/LauncherWorkflowReadiness.md)

Implementation:

- [PortfolioReadinessImplementationPlan.md](./05-Implementation/PortfolioReadinessImplementationPlan.md)

Planned docs:

- `Docs/Architecture/03-Validation/EditorDiagnostics.md`
- `Docs/Architecture/03-Validation/BackendParityReview.md`

## What This Engine Is Not

- Not a finished general-purpose editor with every workflow already productized.
- Not a monolithic renderer where SDK integrations are allowed to define the architecture.
- Not a game-specific codebase where runtime data and rendering backend concerns are mixed together.
- Not a feature zoo optimized for screenshots over reviewability, validation, and extension discipline.
- Not a place where native graphics API usage should leak upward out of backend-private or provider-scoped boundaries.
