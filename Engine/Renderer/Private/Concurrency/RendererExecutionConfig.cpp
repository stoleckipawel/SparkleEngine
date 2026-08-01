#include "PCH.h"

#include "Renderer/Public/Concurrency/RendererExecutionConfig.h"

bool RendererExecutionConfig::IsThreaded() const noexcept
{
	return Mode == RendererExecutionMode::Threaded;
}

bool RendererExecutionConfig::HasAssetTaskRuntime() const noexcept
{
	return AssetTaskExecutor != nullptr &&
	       ApplicationTaskScope != nullptr;
}

std::uint32_t RendererExecutionConfig::GetFrameQueueCapacity() const noexcept
{
	return IsThreaded() ? RenderPipelineDepth + 1u : 1u;
}
