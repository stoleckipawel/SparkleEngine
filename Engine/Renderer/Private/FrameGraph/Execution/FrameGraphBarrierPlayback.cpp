#include "PCH.h"
#include "Renderer/Public/FrameGraph/FrameGraph.h"
#include "Renderer/Public/CommandContext.h"

#include <cassert>

#include "Core/Public/Diagnostics/Log.h"

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

		const ID3D12Resource* beforeResource = ResolveResource(barrier.beforeHandle);
		const ID3D12Resource* afterResource = ResolveResource(barrier.afterHandle);

		if (beforeResource == nullptr || afterResource == nullptr)
		{
			assert(false);
			LOG_WARNING("FrameGraph::EmitCompiledAliasingBarriers: unresolved aliasing barrier resources.");
			continue;
		}

		cmd.AliasResource(const_cast<ID3D12Resource*>(beforeResource), const_cast<ID3D12Resource*>(afterResource));
	}
}

void FrameGraph::EmitCompiledBarriers(CommandContext& cmd, std::string_view passName, const std::vector<CompiledBarrier>& barriers)
	const noexcept
{
	for (const CompiledBarrier& barrier : barriers)
	{
		ID3D12Resource* resource = ResolveResource(barrier.handle);
		if (resource == nullptr)
		{
			assert(false);
			LOG_WARNING("FrameGraph::EmitCompiledBarriers: unresolved resource handle.");
			continue;
		}

		cmd.TransitionResource(resource, barrier.before, barrier.after);
	}
}
