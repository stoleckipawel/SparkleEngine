# Debug View Presentation Precedent

**Status:** research; external-source comparison, not Sparkle architecture or acceptance authority

**Scope:** Unreal Engine, NVIDIA RTXPT/Donut, and AMD Cauldron precedent for view-mode ownership, exposure, tone curves, exact diagnostic presentation, and output conversion

**Local decision owners:** [Render View Modes](ViewModes.md) and [Debug View Presentation Architecture](PresentationArchitecture.md)

**Reference-set context:** [External Renderer Repository Comparison](../../RendererRepositoriesResearch.md)

**Related current readiness:** **40/100** for the existing debug-view feature. The single per-view mode source shape is present but uncompiled; exact-domain presentation and executable proof remain unimplemented/unproved, and external precedent adds no score. See [Current Feature Readiness](../../../../../../Acceptance/CurrentReadiness.md#renderer).

## Unreal Engine

Epic's runtime Engine `EViewModeIndex` is the high-level view-mode contract and includes Lit, Wireframe, Path Tracing, buffer visualization, and other diagnostic modes. Epic documents these values as defining view modes that establish specific show-flag settings, with ordering that matters where the values are serialized. `UGameViewportClient` stores both a view-mode index and engine show flags, so view modes are not merely an Editor enum.

Epic also documents `FEngineShowFlags` as lower-level bits stored with view-family state. That is a separate customization layer: higher-level modes may manipulate flags, and flags are not scalability CVars. The presence of both layers in Unreal does not imply that a smaller engine should create both before it has independent flag consumers.

Transferable lessons:

- a view mode is a runtime rendering semantic, not necessarily an Editor-only enum;
- per-view rendering selection does not read mutable global Editor state;
- lower-level flags are useful only where contributions are independently selectable;
- path tracing can be an ordinary per-viewport rendering mode while sharing the frame and presentation architecture;
- exposure and the tone curve are separate decisions;
- show flags are not scalability or backend-capability policy.

Sparkle adopts the high-level layer now: one `RenderViewMode` shared by Editor and runtime viewport owners. It deliberately defers lower-level show controls until a real orthogonal consumer exists. It should not copy Unreal's full ViewFamily, category surface, visualization registry, dynamic custom flags, or string mutation path.

Primary sources:

- Epic, [`FEngineShowFlags`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/FEngineShowFlags)
- Epic, [`EViewModeIndex`](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/Engine/EViewModeIndex?application_version=5.5)
- Epic, [`UGameViewportClient`](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/UGameViewportClient)
- Epic, [Viewport Toolbar: View Mode and Show Flag Options](https://dev.epicgames.com/documentation/en-us/unreal-engine/viewport-toolbar#viewporttoolbarviewmodeandshowflagoptions)
- Epic, [Viewport Modes](https://dev.epicgames.com/documentation/en-us/unreal-engine/viewport-modes-in-unreal-engine)

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
