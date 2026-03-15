#pragma once

#include "Renderer/Public/RendererAPI.h"
#include "Renderer/Public/SceneData/SceneView.h"

#include "D3D12ConstantBufferData.h"

#include <d3d12.h>

class D3D12SwapChain;
class RenderCamera;
class Scene;
class SceneViewBuilder;
class Window;
class RenderSceneSnapshotCache;

struct SPARKLE_RENDERER_API FrameContext
{
	static FrameContext Build(
	    const Scene& scene,
	    const Window& window,
	    const D3D12SwapChain& swapChain,
	    const RenderCamera& renderCamera,
	    RenderSceneSnapshotCache& renderSceneSnapshotCache,
	    SceneViewBuilder& sceneViewBuilder);

	SceneView sceneView = {};
	PerViewConstantBufferData perViewData = {};
	D3D12_VIEWPORT viewport = {};
	D3D12_RECT scissorRect = {};
};
