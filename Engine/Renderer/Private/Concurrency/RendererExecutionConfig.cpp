#include "PCH.h"

#include "Renderer/Public/Concurrency/RendererExecutionConfig.h"

bool RendererExecutionConfig::IsThreaded() const noexcept
{
	return Mode != RendererExecutionMode::Serial;
}

bool RendererExecutionConfig::HasAssetTaskRuntime() const noexcept
{
	return AssetTaskExecutor != nullptr &&
	       ApplicationTaskScope != nullptr;
}

std::uint32_t RendererExecutionConfig::ResolveFrameSlotCount() const noexcept
{
	return Mode == RendererExecutionMode::ThreadedOneAhead ? 2u : 1u;
}
