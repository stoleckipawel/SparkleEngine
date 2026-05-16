#include "PCH.h"
#include "FrameGraph/FrameGraph.h"
#include "Commands/RenderCommandContext.h"

#include <cassert>

static const auto g_frameGraphBarrierLogger = Logging::GetOrCreateLogger("Renderer.FrameGraph");

void FrameGraph::EmitCompiledBarriers(RenderCommandContext& cmd, const std::vector<FrameGraphBarrier>& barriers) const noexcept
{
	EmitCompiledBarriers(cmd, "Unknown", barriers);
}

void FrameGraph::EmitCompiledAliasingBarriers(RenderCommandContext& cmd, const std::vector<FrameGraphAliasingBarrier>& barriers) const noexcept
{
	EmitCompiledAliasingBarriers(cmd, "Unknown", barriers);
}

void FrameGraph::EmitCompiledAliasingBarriers(
    RenderCommandContext& cmd,
    std::string_view passName,
    const std::vector<FrameGraphAliasingBarrier>& barriers) const noexcept
{
	for (const FrameGraphAliasingBarrier& barrier : barriers)
	{
		assert(barrier.beforeHandle.IsValid());
		assert(barrier.afterHandle.IsValid());

		const NativeResourceHandle beforeResource = ResolveResource(barrier.beforeHandle);
		const NativeResourceHandle afterResource = ResolveResource(barrier.afterHandle);

		if (!beforeResource || !afterResource)
		{
			assert(false);
			SPDLOG_LOGGER_WARN(
			    g_frameGraphBarrierLogger,
			    "FrameGraph::EmitCompiledAliasingBarriers: unresolved aliasing barrier resources.");
			continue;
		}

		cmd.AliasResource(beforeResource, afterResource);
	}
}

void FrameGraph::EmitCompiledBarriers(RenderCommandContext& cmd, std::string_view passName, const std::vector<FrameGraphBarrier>& barriers)
    const noexcept
{
	for (const FrameGraphBarrier& barrier : barriers)
	{
		const NativeResourceHandle resource = ResolveResource(barrier.handle);
		if (!resource)
		{
			assert(false);
			SPDLOG_LOGGER_WARN(g_frameGraphBarrierLogger, "FrameGraph::EmitCompiledBarriers: unresolved resource handle.");
			continue;
		}

		switch (barrier.type)
		{
			case FrameGraphBarrier::Type::Transition:
				cmd.TransitionResource(resource, barrier.before, barrier.after);
				break;
			case FrameGraphBarrier::Type::UnorderedAccess:
				cmd.UnorderedAccessBarrier(resource);
				break;
			default:
				assert(false);
				break;
		}
	}
}
