# Architecture

Status: architecture navigation index

Architecture mirrors the repository's ownership boundaries. Module-owned knowledge lives with its module; only designs that genuinely span several owners live under CrossModule. Code and executable build configuration remain implementation authority.

## Start By Scope

| I need to understand... | Start here |
| --- | --- |
| the repository's major owners and dependency flow | [Whole Repository Architecture Map](WholeRepositoryMap.md) |
| one Engine, Tools, Projects, or build module | [Module Architecture](Modules/README.md) |
| a design or capability spanning multiple modules without one primary owner | [Cross-Module Architecture](CrossModule/README.md) |
| a developer/user journey across build, content, editor, runtime, or delivery | [Product Workflow Coverage](CrossModule/ProductWorkflowCoverage.md) and [Product Execution Traces](CrossModule/ProductExecutionTraces.md) |
| an accepted architectural invariant | [Architecture Decisions](Decisions/README.md) |

## Module Boundaries

- [Engine](Modules/Engine/README.md) — Application, Assets, Core, Editor, GameFramework, Platform, Renderer, RHI, and Tasks.
- [Tools](Modules/Tools/README.md) — Cooking, Launcher, ShaderCompiler, SourceImporters, and shared ToolSupport.
- [Projects](Modules/Projects/README.md) — product-owned composition and workloads, currently Showcase.
- [Build And Packaging](Modules/BuildAndPackaging.md) — repository-wide executable build, staging, installation, and packaging surfaces.

Renderer-specific designs and catalogs are physically located under [Engine/Renderer](Modules/Engine/Renderer/README.md); RHI capability and boundary links are under [Engine/RHI](Modules/Engine/RHI/README.md). Their hyperlinks may cross, but their owning documents remain visibly separate.

## Reading Rule

A repository map answers “where is the current owner?” A module page answers “what source path exists here and what deeper module documents apply?” A decision answers “what invariant has been accepted?” A cross-module document exists only where the responsibility cannot be assigned coherently to one module. None of these prove a build, runtime result, performance result, or release acceptance.
