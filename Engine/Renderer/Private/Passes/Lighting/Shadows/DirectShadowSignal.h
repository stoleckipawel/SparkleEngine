#pragma once

#include "Frame/Graph/RenderFrameGraphTargets.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"
#include "Renderer/Public/FrameGraph/FrameGraphAccelerationStructureHandle.h"

class FrameGraphBuilder;
class RenderRayTracingScene;
struct DirectShadowSignalResources;
struct RenderFrameGraphImportedSceneResources;

void AddDirectShadowSignalPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    const SceneRenderTargets& sceneTargets,
    const GBufferRenderTargets& gbuffer,
	    FrameGraphAccelerationStructureHandle sceneTlas,
	    const DirectShadowSignalResources& shadowSignals,
	    const RenderFrameGraphImportedSceneResources& externalResources,
	    RenderRayTracingScene& rayTracingScene);
