# Cross-Module Plans

**Status:** cross-module plan index

These plans coordinate several durable module owners and have no coherent single-module delivery owner.

## Choose By Outcome

| Planned outcome | Why coordination is cross-module | Current decision boundary |
| --- | --- | --- |
| animated geometry-cache playback | source import, cook, world playback, residency, raster/ray deformation, and RHI lifetime form one feature | target architecture exists; implementation/evidence remain planned |
| trustworthy performance diagnosis | instrumentation, UI, external capture, experiments, evidence, and shipping cost span most runtime owners | external-first staged delivery; no plan phase is acceptance by itself |
| coherent shader authoring-to-runtime system | registration, compilation, artifacts, typed binding, pipelines, reload, and editor workflow span tools/Renderer/RHI | target architecture and detailed clean-break phases remain separate authorities |

Open the Architecture owner first to understand the stable contract, then use the plan for ordering, dependencies, and phase exit criteria.

## Plans

| Plan | Participating owners | Architecture owner |
| --- | --- | --- |
| [Geometry Cache Animation](GeometryCacheAnimation.md) | SourceImporters, Cooking, GameFramework, Renderer, RHI | [Geometry Cache Animation](../../Architecture/CrossModule/GeometryCacheAnimation/README.md) |
| [Performance Diagnostics](PerformanceDiagnostics.md) | Core, Platform, Application, Editor, Renderer, RHI, external tools | [Performance Diagnostics](../../Architecture/CrossModule/PerformanceDiagnostics/README.md) |
| [Shader System](ShaderSystem.md) | Renderer, RHI, ShaderCompiler, Cooking, Editor | [Shader System](../../Architecture/CrossModule/ShaderSystem/README.md) |
