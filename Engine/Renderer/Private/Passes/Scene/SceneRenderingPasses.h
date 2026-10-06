#pragma once

#include "Renderer/Public/Viewport/RenderViewMode.h"
#include <cstdint>

std::uint64_t GetSceneRenderingGraphRebuildKey(RenderViewMode viewMode) noexcept;
