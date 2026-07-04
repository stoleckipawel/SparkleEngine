#pragma once

#include "Renderer/Public/Debug/RenderViewMode.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"
#include "ShaderData/PerFrameConstantBufferData.h"

class Timer;

class PerFrameDataBuilder final
{
  public:
	PerFrameDataBuilder() noexcept = default;
	~PerFrameDataBuilder() noexcept = default;

	PerFrameDataBuilder(const PerFrameDataBuilder&) = delete;
	PerFrameDataBuilder& operator=(const PerFrameDataBuilder&) = delete;
	PerFrameDataBuilder(PerFrameDataBuilder&&) = delete;
	PerFrameDataBuilder& operator=(PerFrameDataBuilder&&) = delete;

	PerFrameConstantBufferData Build(
	    const Timer& timer,
	    RenderViewMode viewMode,
	    RenderViewportExtent sceneExtent) const noexcept;
};
