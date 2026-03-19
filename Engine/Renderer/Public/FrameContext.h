#pragma once

#include "Renderer/Public/RendererAPI.h"
#include "Renderer/Public/SceneData/RenderSceneView.h"

#include "D3D12ConstantBufferData.h"

#include <d3d12.h>

class D3D12SwapChain;
class RenderCamera;
class GameScene;
class RenderSceneViewBuilder;
class Window;

struct SPARKLE_RENDERER_API FrameContext
{
	static FrameContext Build(
	    const GameScene& gameScene,
	    const Window& window,
	    const D3D12SwapChain& swapChain,
	    const RenderCamera& renderCamera,
	    RenderSceneViewBuilder& renderSceneViewBuilder);

	RenderSceneView renderSceneView = {};
	PerViewConstantBufferData perViewData = {};
	D3D12_VIEWPORT viewport = {};
	D3D12_RECT scissorRect = {};
};
