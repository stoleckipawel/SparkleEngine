#pragma once

#include "Renderer/Public/Debug/RenderViewMode.h"

#include <string_view>

namespace RhiSmokeRenderViewModeNames
{
	const char* ToString(RenderViewMode viewMode) noexcept;
	bool TryParse(std::string_view value, RenderViewMode& outViewMode) noexcept;
}
