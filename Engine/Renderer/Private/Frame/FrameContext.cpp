#include "PCH.h"
#include "Renderer/Public/FrameContext.h"

#include "D3D12SwapChain.h"
#include "Renderer/Public/Camera/RenderCamera.h"
#include "Scene/GameScene.h"
#include "SceneData/RenderSceneViewBuilder.h"
#include "Window.h"

FrameContext FrameContext::Build(
    const GameScene& gameScene,
    const Window& window,
    const D3D12SwapChain& swapChain,
    const RenderCamera& renderCamera,
    RenderSceneViewBuilder& renderSceneViewBuilder)
{
	FrameContext frame{};
	const RenderSceneViewportDesc viewportDesc{
	    .camera = &renderCamera,
	    .width = window.GetWidth(),
	    .height = window.GetHeight(),
	};
	frame.renderSceneView = renderSceneViewBuilder.BuildViewport(gameScene, viewportDesc);

	frame.perViewData = renderCamera.GetViewConstantBufferData();
	renderSceneViewBuilder.PopulatePerViewLightingData(frame.renderSceneView, frame.perViewData);
	frame.viewport = swapChain.GetDefaultViewport();
	frame.scissorRect = swapChain.GetDefaultScissorRect();

	return frame;
}
