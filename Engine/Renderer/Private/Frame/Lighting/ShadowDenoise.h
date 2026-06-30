#pragma once

#include "Frame/Core/FrameAssembly.h"
#include "Renderer/Public/Denoising/ShadowDenoiseContract.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;

ShadowDenoiseContract::ShadowDenoiseContract CreateShadowDenoiseFrameResources(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    FrameAssemblyResourceLayout& resources);
