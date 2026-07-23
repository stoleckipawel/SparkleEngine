#pragma once

#include "RendererAPI.h"

#include <cstdint>

enum class RendererExecutionMode : std::uint8_t
{
	Serial = 0,
	ThreadedZeroAhead = 1,
	ThreadedOneAhead = 2,
};

struct SPARKLE_RENDERER_API RendererExecutionConfig final
{
	RendererExecutionMode Mode = RendererExecutionMode::Serial;

	constexpr bool IsThreaded() const noexcept { return Mode != RendererExecutionMode::Serial; }
	std::uint32_t ResolveFrameSlotCount() const noexcept;
};
