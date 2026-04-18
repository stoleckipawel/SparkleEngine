#include "PCH.h"
#include "FrameGraph/FrameGraph.h"
#include "GPU/CommandContext.h"

#include <cassert>

static const auto g_frameGraphBarrierLogger = Engine::Logging::GetOrCreateLogger("Renderer.FrameGraph");

void FrameGraph::EmitCompiledBarriers(CommandContext& cmd, const std::vector<CompiledBarrier>& barriers) const noexcept
{
	EmitCompiledBarriers(cmd, "Unknown", barriers);
}

void FrameGraph::EmitCompiledAliasingBarriers(CommandContext& cmd, const std::vector<CompiledAliasingBarrier>& barriers) const noexcept
{
	EmitCompiledAliasingBarriers(cmd, "Unknown", barriers);
}

void FrameGraph::EmitCompiledAliasingBarriers(
    CommandContext& cmd,
    std::string_view passName,
    const std::vector<CompiledAliasingBarrier>& barriers) const noexcept
{
	for (const CompiledAliasingBarrier& barrier : barriers)
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

void FrameGraph::EmitCompiledBarriers(CommandContext& cmd, std::string_view passName, const std::vector<CompiledBarrier>& barriers)
    const noexcept
{
	for (const CompiledBarrier& barrier : barriers)
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
			case CompiledBarrier::Type::Transition:
				cmd.TransitionResource(resource, barrier.before, barrier.after);
				break;
			case CompiledBarrier::Type::UnorderedAccess:
				cmd.UnorderedAccessBarrier(resource);
				break;
			default:
				assert(false);
				break;
		}
	}
}
