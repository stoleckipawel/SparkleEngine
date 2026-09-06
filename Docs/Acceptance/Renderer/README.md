# Renderer Acceptance

Status: Renderer acceptance progress and orchestration index

This page tracks high-level acceptance routing for features primarily owned by `Engine/Renderer`. [Renderer Architecture](../../Architecture/Modules/Engine/Renderer/Features/README.md) owns feature description and detailed proof contracts; [Feature Completion Reports](../FeatureCompletionReports.md) owns candidate results and approval. This page owns neither detailed criteria nor evidence artifacts.

| Feature/progress route | Feature-owned contract | Result route |
| --- | --- | --- |
| Offline path tracer; `PTD-00` discovery remains blocked and `FCR-REN-08` cannot yet accept oracle status | [Feature dossier](../../Architecture/Modules/Engine/Renderer/Features/Lighting/OfflinePathTracer/README.md) and [discovery gate](../../Architecture/Modules/Engine/Renderer/Features/Lighting/OfflinePathTracer/Discovery.md) | [`FCR-REN-08`](../FeatureCompletionReports.md#initial-completion-report-registry) |
| Debug-view presentation; source present, candidate proof blocked | [Debug Views dossier](../../Architecture/Modules/Engine/Renderer/Features/DebugViews/README.md) and [feature acceptance](../../Architecture/Modules/Engine/Renderer/Features/DebugViews/Acceptance.md) | [`FCR-REN-11`](../FeatureCompletionReports.md#initial-completion-report-registry) |
| Deferred GBuffer decals; not implemented and not admitted to the first release | [Deferred Decals dossier](../../Architecture/Modules/Engine/Renderer/Features/DeferredDecals/README.md) and [feature acceptance](../../Architecture/Modules/Engine/Renderer/Features/DeferredDecals/Acceptance.md) | no candidate report until roadmap admission |
| Exposure; source present, candidate proof open | [Exposure dossier](../../Architecture/Modules/Engine/Renderer/Features/PostProcessing/Exposure.md) | [`FCR-REN-09`](../FeatureCompletionReports.md#initial-completion-report-registry) |
| Image reconstruction/upscaling; source present or capability-gated, candidate proof open | [Image Reconstruction and Upscaling dossier](../../Architecture/Modules/Engine/Renderer/Features/PostProcessing/ImageReconstructionAndUpscaling.md) | [`FCR-REN-10`](../FeatureCompletionReports.md#initial-completion-report-registry) |
| Tone mapping; source present, numerical/color proof open | [Tone Mapping dossier](../../Architecture/Modules/Engine/Renderer/Features/PostProcessing/ToneMapping.md) | [`FCR-REN-14`](../FeatureCompletionReports.md#initial-completion-report-registry) |
| Presentation/output; source present, HDR absent, exact debug presentation partial | [Presentation and Output dossier](../../Architecture/Modules/Engine/Renderer/Features/PostProcessing/PresentationAndOutput.md) | [`FCR-REN-15`](../FeatureCompletionReports.md#initial-completion-report-registry) |
| Color grading, chromatic aberration, and frame generation; not implemented and not admitted | [Post Processing family](../../Architecture/Modules/Engine/Renderer/Features/PostProcessing/README.md) and its negative capability dossiers | no candidate reports until roadmap admission; negative audits `REN-E26` through `REN-E28` |

Other Renderer feature families use the same rule: define their proof beside the feature, then add only a high-level route here when acceptance progress needs coordination across the Renderer portfolio.
