#pragma once

#include "Renderer/Public/Debug/RenderViewMode.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"
#include "ShaderData/PerFrameConstantBufferData.h"

struct TimeInfo;

class PerFrameDataBuilder final
{
  public:
	PerFrameDataBuilder() noexcept;
	~PerFrameDataBuilder() noexcept;

	PerFrameDataBuilder(const PerFrameDataBuilder&) = delete;
	PerFrameDataBuilder& operator=(const PerFrameDataBuilder&) = delete;
	PerFrameDataBuilder(PerFrameDataBuilder&&) = delete;
	PerFrameDataBuilder& operator=(PerFrameDataBuilder&&) = delete;

	PerFrameConstantBufferData Build(
	    const TimeInfo& timing,
	    RenderViewMode viewMode,
	    RenderViewportExtent sceneExtent) const noexcept;
};
