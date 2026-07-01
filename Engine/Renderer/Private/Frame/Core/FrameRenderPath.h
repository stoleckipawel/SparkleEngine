#pragma once

#include <cstdint>

enum class FrameRenderPath : std::uint8_t
{
	RealtimeDeferred,
	PathTracedReference
};

FrameRenderPath ResolveFrameRenderPathFromSettings() noexcept;
