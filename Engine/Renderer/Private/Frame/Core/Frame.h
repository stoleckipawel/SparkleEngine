#pragma once

#include "Frame/Core/FrameAssembly.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;

struct FrameBuildResult
{
	FrameAssemblyResourceLayout Resources = {};
};

FrameBuildResult BuildFrame(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent, bool presentToBackBuffer);
