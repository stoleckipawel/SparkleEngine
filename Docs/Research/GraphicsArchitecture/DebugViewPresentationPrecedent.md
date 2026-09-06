# Debug View Presentation Precedent

Status: research; external-source comparison, not Sparkle architecture or acceptance authority

Scope: Unreal Engine, NVIDIA RTXPT/Donut, and AMD Cauldron precedent used to separate view-mode presets, show flags, exposure, tone curves, exact diagnostic presentation, and output conversion

Local decision owners: [View Modes And Show Flags](../../Architecture/Modules/Engine/Renderer/Features/DebugViews/ViewModesAndShowFlags.md) and [Debug View Presentation Architecture](../../Architecture/Modules/Engine/Renderer/Features/DebugViews/PresentationArchitecture.md)

Reference-set context: [External Renderer Repository Comparison](RendererRepositories.md)

## Unreal Engine

Epic documents `FEngineShowFlags` as bits stored in the view family for artists and developers to customize/debug rendering. View modes are higher-level presets that can manipulate flags, while scalability belongs to console variables. `FSceneViewFamily` owns resolved flags; `FEditorViewportClient` owns current and previous editor-viewport flag sets.

The editor exposes View Mode and Show Flags as neighboring controls. Buffer-visualization records can also carry per-visualization auto-exposure intent rather than assuming every buffer uses the lit presentation path.

Transferable lessons:

- one view owns an immutable resolved flag set; passes do not read mutable global editor state;
- view modes are coherent presets over individual feature switches;
- exposure and the tone curve are separate decisions;
- show flags are not scalability or backend-capability policy.

Sparkle should adopt the ownership/preset separation with a fixed local enum and only implemented consumers. It should not copy Unreal's full category surface, material-driven visualization registry, dynamic custom flags, or string mutation path.

Primary sources:

- Epic, [`FEngineShowFlags`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/FEngineShowFlags)
- Epic, [`FSceneViewFamily`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/FSceneViewFamily)
- Epic, [`FEditorViewportClient`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Editor/UnrealEd/FEditorViewportClient)
- Epic, [Viewport Toolbar: View Mode and Show Flag Options](https://dev.epicgames.com/documentation/en-us/unreal-engine/viewport-toolbar#viewporttoolbarviewmodeandshowflagoptions)
- Epic, [Viewport Show Flags](https://dev.epicgames.com/documentation/en-us/unreal-engine/viewport-show-flags-in-unreal-engine)
- Epic, [`FBufferVisualizationData`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/FBufferVisualizationData)
- Epic, [`FBufferVisualizationData::Record`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/FBufferVisualizationData/Record)

## NVIDIA RTXPT And Donut

RTXPT names pre-tone-map and post-tone-map stages explicitly. Its tone-mapping pass can bypass color grading and the tone curve, but auto exposure is applied outside that branch. Disabling only the curve therefore does not produce an exact visualization.

Transferable lessons:

- tone mapping is an explicit presentation stage with a linear path;
- pre- and post-tone-map domains need clear names;
- exact presentation bypasses exposure and the curve, not one ambiguous `EnableToneMapping` flag.

Sparkle should reuse the stage separation, not RTXPT's sample-level global UI orchestration.

Primary sources at reviewed revisions:

- NVIDIA RTXPT, [`Sample.cpp` at `f08d1c7`](https://github.com/NVIDIA-RTX/RTXPT/blob/f08d1c739071e0faad0c7c274d861124c511abab/Rtxpt/Sample.cpp#L2189-L2208)
- NVIDIA RTXPT, [`ToneMapping.ps.hlsli` at `f08d1c7`](https://github.com/NVIDIA-RTX/RTXPT/blob/f08d1c739071e0faad0c7c274d861124c511abab/Rtxpt/ToneMapper/ToneMapping.ps.hlsli#L133-L172)
- NVIDIA Donut, [`ToneMappingPasses.cpp` at `bfdebdd`](https://github.com/NVIDIA-RTX/Donut/blob/bfdebdd7dd5455c503b2737a1967a4ef651c145b/src/render/ToneMappingPasses.cpp)

## AMD Cauldron

Cauldron's reviewed tone-mapping shader has a linear operator that still applies exposure and a separate raw pass-through when exposure is negative. Its color-conversion shader separately applies the target display transform and transfer function.

Transferable lessons:

- exposure, tone curve, and output conversion are separate concerns;
- a linear tone mapper is not exact pass-through when exposure still changes the signal;
- display conversion remains required after either mapping choice.

Sparkle should adopt the separation, not the sentinel exposure value, numeric tone-mapper switch, or backend-specific duplication.

Primary sources at the revision pinned by Sparkle's renderer research:

- AMD Cauldron, [`Tonemapping.hlsl` at `b92d559`](https://github.com/GPUOpen-LibrariesAndSDKs/Cauldron/blob/b92d559bd083f44df9f8f42a6ad149c1584ae94c/src/DX12/shaders/Tonemapping.hlsl)
- AMD Cauldron, [`ColorConversionPS.hlsl` at `b92d559`](https://github.com/GPUOpen-LibrariesAndSDKs/Cauldron/blob/b92d559bd083f44df9f8f42a6ad149c1584ae94c/src/DX12/shaders/ColorConversionPS.hlsl)
