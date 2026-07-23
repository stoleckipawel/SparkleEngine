#include "PCH.h"

#include "Renderer/Public/Concurrency/RendererExecutionConfig.h"

std::uint32_t RendererExecutionConfig::ResolveFrameSlotCount() const noexcept
{
	return Mode == RendererExecutionMode::ThreadedOneAhead ? 2u : 1u;
}
