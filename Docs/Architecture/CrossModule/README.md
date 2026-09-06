# Cross-Module Architecture

Status: cross-module architecture index

Use this folder only when a system has several durable owners and no single module can own the whole contract. Each document must name its participating modules and link back to their module pages. A relationship between two modules alone does not justify moving their module-owned knowledge here.

## Runtime And Content Systems

| Document | Participating owners | Read it when... |
| --- | --- | --- |
| [Geometry Cache Animation](GeometryCacheAnimation.md) | SourceImporters, Cooking, GameFramework, Renderer, RHI | understanding the target import-to-render animation pipeline |
| [Geometry Cache Capability](GeometryCacheAnimationCapability.md) | the same end-to-end path | checking the dated source-present seams and explicit gaps |
| [Shader System](ShaderSystem.md) | Renderer, RHI, ShaderCompiler, Cooking, Editor | understanding authoring, compilation, delivery, loading, and execution as one system |
| [Multithreaded Engine](MultithreadedEngine.md) | Tasks plus runtime, Renderer, RHI, and tools | understanding shared threading, publication, shutdown, and failure boundaries |
| [Product Workflow Coverage](ProductWorkflowCoverage.md) | Application, Editor, Launcher, tools, content, build, Showcase, and delivery | comparing developer/user journeys horizontally, including incomplete packaging and support paths |
| [Product Execution Traces](ProductExecutionTraces.md) | the same product and tool owners | tracing vital non-graphics workflows vertically from request through result, failure, recovery, and settlement |

## Graphics And Diagnostics Cross-Cuts

| Document | Participating owners | Read it when... |
| --- | --- | --- |
| [Graphics Feature Coverage Matrix](GraphicsCoverageMatrix.md) | Renderer, RHI, shaders, tools, and product selectors | comparing feature coverage horizontally across backends and paths |
| [Graphics Feature Execution Traces](FeatureExecutionTraces.md) | Application, GameFramework, Renderer, RHI, ShaderCompiler | tracing selected features vertically from producer to consumer |
| [Performance Diagnostics](PerformanceDiagnostics.md) | Core, Platform, Application, Editor, Renderer, RHI, and external tools | understanding the target diagnostics ownership model |
| [Performance Diagnostics Capability](PerformanceDiagnosticsCapability.md) | the same diagnostic surfaces | checking the dated source-present capability and gaps |
| [Strategy Coverage](StrategyCoverage.md) | all inventoried modules | reconciling module inventories with persona, roadmap, and gap requirements |

Renderer-owned feature designs remain under [Engine/Renderer](../Modules/Engine/Renderer/README.md), even when they consume RHI services.
