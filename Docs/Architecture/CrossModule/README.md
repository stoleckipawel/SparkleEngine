# Cross-Module Architecture

**Status:** cross-module architecture index

**Current state:** this index is not a single feature score. Its owned feature dossiers range from **0/100** target-only geometry-cache/neural paths, through **20/100** performance diagnostics, to a **50/100** integrated shader-system route. See [Current Feature Readiness](../../Acceptance/CurrentReadiness.md#explicit-missing-or-not-yet-admitted-capabilities).

Use this folder only when a system has several durable owners and no single module can own the whole contract. Each subject must name its participating modules and link back to their module pages. Its plan, research, capability snapshot, and local proof contract stay beside its dossier. A relationship between two modules alone does not justify moving their module-owned knowledge here.

## At A Glance

| Cross-cut | Why no single module owns it | First useful view |
| --- | --- | --- |
| runtime/content pipelines | import/cook/world/Renderer/RHI or shader/tool/runtime lifecycles form one product result | feature dossier and its owner/lifetime diagram |
| engine concurrency | tasks, application, world, Renderer, RHI, and tools publish through different threads/queues | shared topology and shutdown contract |
| product journeys | launcher/build/tools/content/application/delivery combine for one actor outcome | horizontal workflow matrix then vertical trace |
| graphics coverage | Renderer semantics cross shader, RHI backend, selector, and evidence boundaries | backend/mode matrix then selected execution trace |
| performance diagnostics | measurement, UI, external tools, evidence, and shipping cost cross many owners | capability/delivery split and bounded data flow |
| documentation traceability | strategy, plans, acceptance, research, and Architecture must route without duplicate truth | coverage ledgers, then the owning dossier |

Cross-module pages own the *join*. They must name what each participant contributes and must not absorb the participant's module-local mechanics.

## Runtime And Content Systems

| Document | Participating owners | Read it when... |
| --- | --- | --- |
| [Geometry Cache Animation](GeometryCacheAnimation/README.md) | SourceImporters, Cooking, GameFramework, Renderer, RHI | entering the feature dossier for its architecture, capability, plan, and local completion contract |
| [Shader System](ShaderSystem/README.md) | Renderer, RHI, ShaderCompiler, Cooking, Editor | entering the feature dossier for architecture, plan, research, migration provenance, and local completion contract |
| [Neural Graphics](NeuralGraphics/README.md) | future training/export tools, Assets, Renderer, RHI, Showcase, Build/Packaging | distinguishing current vendor inference from the absent owned training, model-to-kernel, and runtime feature |
| [Multithreaded Engine](MultithreadedEngine.md) | Tasks plus runtime, Renderer, RHI, and tools | understanding shared threading, publication, shutdown, and failure boundaries |
| [First Release Implementation Plan](FirstRelease/README.md) | every product, module, feature, build, evidence, and release owner | selecting the next release stage and following it into the module-owned work package |
| [Product Workflow Coverage](ProductWorkflowCoverage.md) | Application, Editor, Launcher, tools, content, build, Showcase, and delivery | comparing developer/user journeys horizontally, including incomplete packaging and support paths |
| [Product Execution Traces](ProductExecutionTraces.md) | the same product and tool owners | tracing vital non-graphics workflows vertically from request through result, failure, recovery, and settlement |

## Graphics And Diagnostics Cross-Cuts

| Document | Participating owners | Read it when... |
| --- | --- | --- |
| [Graphics Feature Coverage Matrix](GraphicsCoverageMatrix.md) | Renderer, RHI, shaders, tools, and product selectors | comparing feature coverage horizontally across backends and paths |
| [Graphics Feature Execution Traces](FeatureExecutionTraces.md) | Application, GameFramework, Renderer, RHI, ShaderCompiler | tracing selected features vertically from producer to consumer |
| [Performance Diagnostics](PerformanceDiagnostics/README.md) | Core, Platform, Application, Editor, Renderer, RHI, and external tools | entering the feature dossier for its architecture, capability, plan, research, and local completion contract |
| [Strategy Coverage](StrategyCoverage.md) | all inventoried modules | reconciling module inventories with persona, roadmap, and gap requirements |
| [Feature Documentation Coverage](FeatureDocumentation/README.md) | all strategy, acceptance, plan, research, and Architecture owners | auditing every named source document and stable target/report identifier back to one feature dossier |

Renderer-owned feature designs remain under [Engine/Renderer](../Modules/Engine/Renderer/README.md), even when they consume RHI services.
