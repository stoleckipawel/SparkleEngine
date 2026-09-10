# Renderer Acceptance

**Status:** Renderer acceptance progress and orchestration index

This page tracks high-level acceptance routing for features primarily owned by `Engine/Renderer`. [Renderer Architecture](../../Architecture/Modules/Engine/Renderer/Features/README.md) owns feature description and detailed proof contracts; [Feature Completion Reports](../FeatureCompletionReports.md) owns candidate results and approval. This page owns neither detailed criteria nor evidence artifacts.

**Current readiness:** **36/100** across 26 tracked Renderer families; all remain Blocked, with verification and delivery/adoption at zero. Four admitted release features are currently **0/100**. See [Current Feature Readiness](../CurrentReadiness.md#renderer).

## At A Glance

| Portfolio state | Feature families |
| --- | --- |
| source path exists but candidate proof remains open | debug views, exposure, reconstruction/upscaling, tone mapping, presentation/output, pipeline materialization, residency, temporal state, settings, latency coordination, visibility, and resolution/sampling |
| development contract ready for repository-owner ratification; implementation and acceptance have not begun | independent Reference Path Tracer oracle (`PTD-00-R1`) |
| not implemented but admitted to first release | deferred decals, color grading, chromatic aberration, and HDR10 display output |
| not implemented and not admitted to first release | frame generation |

No Renderer row is promoted by this summary. Follow the feature contract for what must pass and the `FCR-REN-*` result route for what a specific candidate actually proved.

## Progress Routes

| Feature/progress route | Readiness | Feature-owned contract | Result route |
| --- | ---: | --- | --- |
| Reference Path Tracer; `PTD-00-R1` awaits immutable repository-owner ratification, no implementation evidence exists, and `FCR-REN-08` cannot yet accept oracle status | **20/100** | [Feature dossier](../../Architecture/Modules/Engine/Renderer/Features/Lighting/ReferencePathTracer/README.md), [target architecture](../../Architecture/Modules/Engine/Renderer/Features/Lighting/ReferencePathTracer/ExecutionArchitecture.md), and [conditional plan](../../Architecture/Modules/Engine/Renderer/Features/Lighting/ReferencePathTracer/Plan.md) | [`FCR-REN-08`](../FeatureCompletionReports.md#initial-completion-report-registry) |
| Debug-view presentation; source present, candidate proof blocked | **40/100** | [Debug Views dossier](../../Architecture/Modules/Engine/Renderer/Features/DebugViews/README.md) and [feature acceptance](../../Architecture/Modules/Engine/Renderer/Features/DebugViews/Acceptance.md) | [`FCR-REN-11`](../FeatureCompletionReports.md#initial-completion-report-registry) |
| Deferred GBuffer decals; mandatory target, implementation not found | **0/100** | [Deferred Decals dossier](../../Architecture/Modules/Engine/Renderer/Features/DeferredDecals/README.md) and [feature acceptance](../../Architecture/Modules/Engine/Renderer/Features/DeferredDecals/Acceptance.md) | [`FCR-REN-23`](../FeatureCompletionReports.md#initial-completion-report-registry) |
| Exposure; source present, candidate proof open | **45/100** | [Exposure dossier](../../Architecture/Modules/Engine/Renderer/Features/PostProcessing/DisplayPipeline/Exposure.md) | [`FCR-REN-09`](../FeatureCompletionReports.md#initial-completion-report-registry) |
| Image reconstruction/upscaling; source present or capability-gated, candidate proof open | **40/100** | [Image Reconstruction and Upscaling dossier](../../Architecture/Modules/Engine/Renderer/Features/PostProcessing/ReconstructionAndGeneration/ImageReconstructionAndUpscaling.md) | [`FCR-REN-10`](../FeatureCompletionReports.md#initial-completion-report-registry) |
| Tone mapping; source present, numerical/color proof open | **45/100** | [Tone Mapping dossier](../../Architecture/Modules/Engine/Renderer/Features/PostProcessing/DisplayPipeline/ToneMapping.md) | [`FCR-REN-14`](../FeatureCompletionReports.md#initial-completion-report-registry) |
| Presentation/output; source present, HDR absent, exact debug presentation partial | **45/100** | [Presentation and Output dossier](../../Architecture/Modules/Engine/Renderer/Features/PostProcessing/DisplayPipeline/PresentationAndOutput.md) | [`FCR-REN-15`](../FeatureCompletionReports.md#initial-completion-report-registry) |
| Pipeline materialization and typed binding; source present, ABI/native/reload proof open | **50/100** | [Pipeline Materialization dossier](../../Architecture/Modules/Engine/Renderer/Features/ShaderRuntime/PipelineMaterializationAndTypedBinding.md) | [`FCR-REN-16`](../FeatureCompletionReports.md#initial-completion-report-registry) |
| Mesh and texture residency; source present, pressure/failure/retirement proof open | **50/100** | [Mesh and Texture Residency dossier](../../Architecture/Modules/Engine/Renderer/Features/GeometryAndResources/MeshAndTextureResidency.md) | [`FCR-REN-17`](../FeatureCompletionReports.md#initial-completion-report-registry) |
| Temporal sampling and history; source present, exact/multi-view/consumer proof open | **45/100** | [Temporal Sampling and History dossier](../../Architecture/Modules/Engine/Renderer/Features/FrameExecution/TemporalSamplingAndHistory.md) | [`FCR-REN-18`](../FeatureCompletionReports.md#initial-completion-report-registry) |
| Settings state and persistence; source present with known durability/diagnostic gaps | **40/100** | [Settings State and Persistence dossier](../../Architecture/Modules/Engine/Renderer/Features/RuntimeConfiguration/SettingsStateAndPersistence.md) | [`FCR-REN-19`](../FeatureCompletionReports.md#initial-completion-report-registry) |
| Latency coordination; capability-gated D3D12 route present, identity/failure/benefit proof open | **25/100** | [Latency Coordination dossier](../../Architecture/Modules/Engine/Renderer/Features/FrameExecution/LatencyCoordination.md) | [`FCR-REN-20`](../FeatureCompletionReports.md#initial-completion-report-registry) |
| Visibility and draw preparation; source present, correctness/equivalence/performance proof open | **45/100** | [Visibility and Draw Preparation dossier](../../Architecture/Modules/Engine/Renderer/Features/GeometryAndResources/VisibilityAndDrawPreparation.md) | [`FCR-REN-21`](../FeatureCompletionReports.md#initial-completion-report-registry) |
| Resolution, sampling, and anti-aliasing; extent/single-sample/provider paths present with explicit absent AA/dynamic-resolution modes | **40/100** | [Resolution, Sampling, and Anti-Aliasing dossier](../../Architecture/Modules/Engine/Renderer/Features/PostProcessing/ReconstructionAndGeneration/ResolutionSamplingAndAntiAliasing.md) | [`FCR-REN-22`](../FeatureCompletionReports.md#initial-completion-report-registry) |
| Color grading; mandatory target, implementation not found | **0/100** | [Color Grading package](../../Architecture/Modules/Engine/Renderer/Features/PostProcessing/DisplayPipeline/ColorGrading/README.md) | [`FCR-REN-24`](../FeatureCompletionReports.md#initial-completion-report-registry) |
| Chromatic aberration; mandatory target, implementation not found | **0/100** | [Chromatic Aberration package](../../Architecture/Modules/Engine/Renderer/Features/PostProcessing/DisplayPipeline/ChromaticAberration/README.md) | [`FCR-REN-25`](../FeatureCompletionReports.md#initial-completion-report-registry) |
| HDR display output; mandatory target, implementation not found and discovery open | **0/100** | [HDR Display Output package](../../Architecture/Modules/Engine/Renderer/Features/PostProcessing/DisplayPipeline/HDRDisplayOutput/README.md) | [`FCR-REN-26`](../FeatureCompletionReports.md#initial-completion-report-registry) |
| Frame generation; not implemented and not admitted | **0/100** | [Frame Generation dossier](../../Architecture/Modules/Engine/Renderer/Features/PostProcessing/ReconstructionAndGeneration/FrameGeneration.md) | no candidate report until roadmap admission; negative audit `REN-E28` |

Other Renderer feature families use the same rule: define their proof beside the feature, then add only a high-level route here when acceptance progress needs coordination across the Renderer portfolio.
