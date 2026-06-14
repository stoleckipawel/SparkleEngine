#pragma once

#include "Core/Public/Console/CVar.h"
#include "Renderer/Public/Debug/RenderViewMode.h"
#include "Renderer/Public/RendererAPI.h"

extern SPARKLE_RENDERER_API ConsoleVariable<RenderViewMode> CVarRenderViewMode;
extern SPARKLE_RENDERER_API ConsoleVariable<bool> CVarRendererMeshAutoBatching;
extern SPARKLE_RENDERER_API ConsoleVariable<std::uint32_t> CVarRayTracingPartitionsPerAxis;
extern SPARKLE_RENDERER_API ConsoleVariable<bool> CVarRayTracingGlobalPartition;
