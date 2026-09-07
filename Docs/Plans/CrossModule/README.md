# Cross-Module Plans

**Status:** cross-module plan index

These plans coordinate several durable module owners and have no coherent single-module delivery owner.

The current baselines are **0/100** for geometry-cache animation, **20/100** for the performance-diagnostics product, and **50/100** for the integrated shader-system route. These are feature scores, not plan completion. See [Current Feature Readiness](../../Acceptance/CurrentReadiness.md#explicit-missing-or-not-yet-admitted-capabilities).

## Choose By Outcome

| Planned outcome | Current feature readiness | Why coordination is cross-module | Current decision boundary |
| --- | ---: | --- | --- |
| animated geometry-cache playback | **0/100** | source import, cook, world playback, residency, raster/ray deformation, and RHI lifetime form one feature | target architecture exists; implementation/evidence remain planned |
| trustworthy performance diagnosis | **20/100** | instrumentation, UI, external capture, experiments, evidence, and shipping cost span most runtime owners | instrumentation foundations exist; target product and proof remain planned |
| coherent shader authoring-to-runtime system | **50/100** | registration, compilation, artifacts, typed binding, pipelines, reload, and editor workflow span tools/Renderer/RHI | integrated source route exists; candidate verification and delivery are zero |

Open the Architecture owner first to understand the stable contract, then use the plan for ordering, dependencies, and phase exit criteria.

## Plans

| Plan | Participating owners | Architecture owner |
| --- | --- | --- |
| [Geometry Cache Animation](GeometryCacheAnimation.md) | SourceImporters, Cooking, GameFramework, Renderer, RHI | [Geometry Cache Animation](../../Architecture/CrossModule/GeometryCacheAnimation/README.md) |
| [Performance Diagnostics](PerformanceDiagnostics.md) | Core, Platform, Application, Editor, Renderer, RHI, external tools | [Performance Diagnostics](../../Architecture/CrossModule/PerformanceDiagnostics/README.md) |
| [Shader System](ShaderSystem.md) | Renderer, RHI, ShaderCompiler, Cooking, Editor | [Shader System](../../Architecture/CrossModule/ShaderSystem/README.md) |
