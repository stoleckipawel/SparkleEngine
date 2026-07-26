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
	bool EnableEditorRenderPackets = false;
	TaskExecutor* AssetTaskExecutor = nullptr;
	TaskScope* ApplicationTaskScope = nullptr;

	constexpr bool IsThreaded() const noexcept { return Mode != RendererExecutionMode::Serial; }
	constexpr bool HasAssetTaskRuntime() const noexcept
	{
		return AssetTaskExecutor != nullptr && ApplicationTaskScope != nullptr;
	}
	std::uint32_t ResolveFrameSlotCount() const noexcept;
};
