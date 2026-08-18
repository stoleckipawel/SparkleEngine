#pragma once

#include "Renderer/Public/Debug/RenderViewMode.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"
#include "ShaderData/PerFrameConstantBufferData.h"

#include <cstdint>

struct RenderFrameTime;

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
	    std::uint64_t frameId,
	    const RenderFrameTime& time,
	    RenderViewMode viewMode,
	    RenderViewportExtent sceneExtent) const noexcept;
};
