#pragma once

#include "RendererAPI.h"

#include <cstdint>

class TaskExecutor;
class TaskScope;

enum class RendererExecutionMode : std::uint8_t
{
	Serial = 0,
	ThreadedZeroAhead = 1,
	ThreadedOneAhead = 2,
};

struct SPARKLE_RENDERER_API RendererExecutionConfig final
{
	RendererExecutionMode Mode = RendererExecutionMode::Serial;
	bool EnableUiRenderPackets = false;
	TaskExecutor* AssetTaskExecutor = nullptr;
	TaskScope* ApplicationTaskScope = nullptr;

	bool IsThreaded() const noexcept;
	bool HasAssetTaskRuntime() const noexcept;
	std::uint32_t ResolveFrameSlotCount() const noexcept;
};
