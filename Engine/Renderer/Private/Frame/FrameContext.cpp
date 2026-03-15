#include "PCH.h"
#include "Renderer/Public/FrameContext.h"

#include "D3D12SwapChain.h"
#include "Renderer/Public/Camera/RenderCamera.h"
#include "Scene/Scene.h"
#include "SceneData/RenderSceneSnapshot.h"
#include "SceneData/RenderSceneSnapshotCache.h"
#include "SceneData/SceneViewBuilder.h"
#include "Window.h"

FrameContext FrameContext::Build(
    const Scene& scene,
    const Window& window,
    const D3D12SwapChain& swapChain,
    const RenderCamera& renderCamera,
    RenderSceneSnapshotCache& renderSceneSnapshotCache,
    SceneViewBuilder& sceneViewBuilder)
{
	const RenderSceneSnapshot& sceneSnapshot = renderSceneSnapshotCache.Capture(scene);

	FrameContext frame{};
	frame.sceneView = sceneViewBuilder.Build(sceneSnapshot, renderCamera, window.GetWidth(), window.GetHeight());

	frame.perViewData = renderCamera.GetViewConstantBufferData();
	frame.perViewData.SunDirection = frame.sceneView.sunLight.direction;
	frame.perViewData.SunIntensity = frame.sceneView.sunLight.intensity;
	frame.perViewData.SunColor = frame.sceneView.sunLight.color;
	frame.viewport = swapChain.GetDefaultViewport();
	frame.scissorRect = swapChain.GetDefaultScissorRect();

	return frame;
}
