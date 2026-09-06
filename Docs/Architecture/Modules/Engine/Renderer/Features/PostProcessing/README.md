# Renderer Post Processing

Status: current feature-family dossier; source-backed taxonomy, not visual-quality, latency, performance, backend, or release evidence

Verified: 2026-09-06 against source revision `d236da11`; `Engine/Renderer` is unchanged from the earlier `8414b5dc` source audit

Scope: the image-space stages that turn scene-linear lighting into a resolved display image, including explicit negative coverage for commonly expected stages that Sparkle does not currently implement

## Family Promise

Sparkle has one ordered post-processing path. Each stage owns one transformation and may not be inferred from a neighboring stage:

```text
SceneColor at render extent
  -> Exposure measurement/history
  -> optional Ray Reconstruction
  -> Upscaling when reconstruction did not produce ResolvedSceneColor
  -> optional Debug View replacement
  -> Tone Mapping
  -> Output Encoding
  -> back-buffer copy or viewport product
```

Color grading, chromatic aberration, and frame generation do not enter this path today. Their documents are negative capability boundaries so SDK presence, tone-mapper curves, temporal upscaling, or Reflex latency markers cannot be mistaken for those features.

## Folder Map

| Feature family | Why it is grouped here |
| --- | --- |
| [Reconstruction And Generation](ReconstructionAndGeneration/README.md) | resolution/sample policy, reconstruction/upscaling, and the distinct absent generated-frame capability own pre-display image production |
| [Display Pipeline](DisplayPipeline/README.md) | exposure, tone/display transforms, explicitly absent display effects, encoding, and publication own scene-to-display output |

## Feature Map

| Feature | Capability IDs | Current state | Owning dossier |
| --- | --- | --- | --- |
| Exposure | `REN-POST-01` through `REN-POST-03` | Implemented path; proof open | [Exposure](DisplayPipeline/Exposure.md) |
| Image reconstruction and upscaling | `REN-POST-04` through `REN-POST-06` | Linear implemented; NVIDIA providers capability-gated; proof open | [Image Reconstruction and Upscaling](ReconstructionAndGeneration/ImageReconstructionAndUpscaling.md) |
| Resolution, sampling, and anti-aliasing | `REN-RESO-01` through `REN-RESO-07` plus shared `REN-TEMP-*`/provider boundaries | output/render extent and single-sample/Halton path implemented; MSAA, standalone post AA, and dynamic resolution not found | [Resolution, Sampling, and Anti-Aliasing](ReconstructionAndGeneration/ResolutionSamplingAndAntiAliasing.md) |
| Tone mapping | `REN-POST-07` | Three selectable operators; proof open | [Tone Mapping](DisplayPipeline/ToneMapping.md) |
| Output and presentation | `REN-POST-08` through `REN-POST-10` | SDR-oriented encoding/publication implemented; HDR absent; exact debug presentation partial | [Presentation and Output](DisplayPipeline/PresentationAndOutput.md) |
| Color grading | `REN-POST-11` | Not found | [Color Grading](DisplayPipeline/ColorGrading.md) |
| Chromatic aberration | `REN-POST-12` | Not found | [Chromatic Aberration](DisplayPipeline/ChromaticAberration.md) |
| Frame generation | `REN-POST-13` | Not found; Reflex is not frame generation | [Frame Generation](ReconstructionAndGeneration/FrameGeneration.md) |

## Ordering And Ownership Invariants

- Exposure reads the scene-linear lighting result before debug replacement and supplies one per-view multiplier to reconstruction/upscaling and tone mapping.
- Ray Reconstruction and upscaling are mutually exclusive producers of `ResolvedSceneColor`; the frame does not intentionally resolve twice.
- Output extent, provider-resolved render extent, temporal sampling, and attachment sample count remain separate concepts. [Resolution, Sampling, and Anti-Aliasing](ReconstructionAndGeneration/ResolutionSamplingAndAntiAliasing.md) owns their combination and negative-mode boundary.
- Debug visualization may replace resolved color before the common tone/output path, but [Debug Views](../DebugViews/README.md) owns visualization semantics.
- Tone mapping owns HDR scene-referred to display-linear mapping. It does not own color grading, output transfer encoding, or publication.
- Presentation owns output encoding and destination publication. [UI and Viewport Composition](../ViewportAndDiagnostics/UiAndViewportComposition.md) happens after graph execution and is not a post-processing effect.
- Frame generation would create additional presented frames and therefore requires pacing, latency, UI, swapchain, and provenance contracts beyond ordinary upscaling. No such owner exists today.

## Shared Failure And Completion Boundary

- Requested and active provider/state must be distinguishable; unsupported external paths may not remain advertised as active.
- Extent, view, scene, provider, shader, and graph-generation changes must invalidate only the affected temporal state and retire old resources after their queue tokens complete.
- Every stage must state its input/output color domain, extent, format, alpha behavior, history, and failure result. A stage with no current implementation stays explicitly unavailable.
- `REN-E13` through `REN-E18` and `REN-E26` through `REN-E28` own the currently planned evidence. Candidate verdicts belong in [Feature Completion Reports](../../../../../../Acceptance/FeatureCompletionReports.md).
- The implemented child dossiers define stable feature-local `AC-*`, `FM-*`, and `CHK-*` criteria under `INV-009`; those criteria are unproved until candidate execution. The negative dossiers define current absence rather than implementation plans.

## Family-Level Completion Contract

- `AC-POST-01` — exactly one stage owns each transition in the documented order, and every edge records input/output color domain, extent, format, alpha, identity, and history behavior.
- `AC-POST-02` — every reachable combination of exposure, reconstruction/upscaler, debug mode, tone mapper, encoding, output target, and backend either produces the one declared result or exposes a documented unavailable/fallback state.
- `AC-POST-03` — scene/view/extent/provider/shader/topology changes reset only affected temporal stages, never bind mixed generations, and retire old resources after queue completion.
- `AC-POST-04` — absent color grading, chromatic aberration, frame generation, and HDR-display features remain absent from selectors, graph passes, shader/provider registration, UI, and product claims.

| Failure ID | Injection or cause | Required safe behavior | Detecting check |
| --- | --- | --- | --- |
| `FM-POST-01` | two stages claim `ResolvedSceneColor` or a color-domain/extent edge is missing | topology/contract audit rejects the graph/change before evidence use | `CHK-POST-01` |
| `FM-POST-02` | provider/debug/output combination is unavailable or changes generation mid-flight | active state reports fallback/refusal; no stale or double-processed product publishes | `CHK-POST-02` |
| `FM-POST-03` | neighboring functionality or SDK vocabulary is advertised as an absent feature | selector/source/package audit fails `AC-POST-04` | `CHK-POST-03` |

| Check | Exercise and oracle | Covers |
| --- | --- | --- |
| `CHK-POST-01` | enumerate the graph and resource contracts for every stage; assert one producer and declared domain/extent/format/history at each edge | `AC-POST-01`; `FM-POST-01` |
| `CHK-POST-02` | pairwise covering matrix of implemented selectors across resize, camera cut, provider failure, dual viewports, and D3D12/Vulkan; inspect active state and decoded products | `AC-POST-02`, `AC-POST-03`; `FM-POST-02` |
| `CHK-POST-03` | search selectors, CMake, shader registrations, provider evaluation, passes, UI, and package manifests for each negative capability and rejected near-synonym | `AC-POST-04`; `FM-POST-03` |

The family passes only when every applicable child contract passes and `CHK-POST-*` proves the joins. A passing child cannot compensate for an unowned or double-applied transition.

## Primary Source Routes

- [`PostProcessing.cpp`](../../../../../../../Engine/Renderer/Private/Passes/PostProcessing/PostProcessing.cpp)
- [`BuildRenderFrameGraph.cpp`](../../../../../../../Engine/Renderer/Private/Frame/Graph/BuildRenderFrameGraph.cpp)
- [`Presentation.cpp`](../../../../../../../Engine/Renderer/Private/Passes/Presentation/Presentation.cpp)
- [`RendererImageProviderStack.cpp`](../../../../../../../Engine/Renderer/Private/Providers/RendererImageProviderStack.cpp)
