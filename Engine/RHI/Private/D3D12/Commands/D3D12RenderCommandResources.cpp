#include "PCH.h"

#include "D3D12/Commands/D3D12RenderCommandList.h"

#include "D3D12/D3D12TypeConversions.h"
#include "Core/Public/Diagnostics/Verify.h"

static const auto g_d3d12RenderCommandListLogger = Logging::GetOrCreateLogger("RHI.D3D12.CommandList");

void D3D12RenderCommandList::CopyResource(RhiResourceHandle destinationResource, RhiResourceHandle sourceResource) noexcept
{
	if (m_commandList == nullptr || !destinationResource || !sourceResource)
	{
		Diagnostics::Fatal(
		    g_d3d12RenderCommandListLogger,
		    __FILE__,
		    __LINE__,
		    "D3D12 CopyResource requires an active command list and two valid resources.");
	}
	TrackResource(destinationResource);
	TrackResource(sourceResource);
	m_commandList->CopyResource(D3D12TypeConversions::ToResource(destinationResource), D3D12TypeConversions::ToResource(sourceResource));
}

void D3D12RenderCommandList::AliasResource(RhiResourceHandle beforeResource, RhiResourceHandle afterResource) noexcept
{
	if (m_commandList == nullptr || !beforeResource || !afterResource)
	{
		Diagnostics::Fatal(
		    g_d3d12RenderCommandListLogger,
		    __FILE__,
		    __LINE__,
		    "D3D12 aliasing barriers require an active command list and two valid resources.");
	}

	TrackResource(beforeResource);
	TrackResource(afterResource);

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Aliasing.pResourceBefore = D3D12TypeConversions::ToResource(beforeResource);
	barrier.Aliasing.pResourceAfter = D3D12TypeConversions::ToResource(afterResource);
	m_commandList->ResourceBarrier(1, &barrier);
}

void D3D12RenderCommandList::TransitionResource(RhiResourceHandle resource, ResourceState before, ResourceState after) noexcept
{
	if (m_commandList == nullptr || !resource)
	{
		Diagnostics::Fatal(
		    g_d3d12RenderCommandListLogger,
		    __FILE__,
		    __LINE__,
		    "D3D12 resource transitions require an active command list and a valid resource.");
	}
	if (before == after)
	{
		return;
	}
	TrackResource(resource);

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = D3D12TypeConversions::ToResource(resource);
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = ResolveResourceState(before);
	barrier.Transition.StateAfter = ResolveResourceState(after);
	if (barrier.Transition.StateBefore == barrier.Transition.StateAfter)
	{
		return;
	}
	m_commandList->ResourceBarrier(1, &barrier);
}

void D3D12RenderCommandList::UnorderedAccessBarrier(RhiResourceHandle resource) noexcept
{
	if (m_commandList == nullptr || !resource)
	{
		Diagnostics::Fatal(
		    g_d3d12RenderCommandListLogger,
		    __FILE__,
		    __LINE__,
		    "D3D12 unordered-access barriers require an active command list and a valid resource.");
	}
	TrackResource(resource);

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.UAV.pResource = D3D12TypeConversions::ToResource(resource);
	m_commandList->ResourceBarrier(1, &barrier);
}

D3D12_RESOURCE_STATES D3D12RenderCommandList::ResolveResourceState(ResourceState state) const noexcept
{
	if (m_queueType == ERhiQueueType::Compute && state == ResourceState::ShaderResource)
	{
		return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	}
	return D3D12TypeConversions::ToResourceStates(state);
}
