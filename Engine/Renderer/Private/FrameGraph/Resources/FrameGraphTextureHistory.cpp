#include "PCH.h"
#include "FrameGraph/FrameGraph.h"

#include "FrameGraph/ResourceUsage.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "RHI/Public/Memory/RhiMemoryTypes.h"
#include "RHI/Public/Resources/RhiResourceDesc.h"
#include "RHI/Public/Resources/RhiResourceService.h"

#include <algorithm>
#include <string>

class FrameGraphTextureHistoryOperations final
{
  public:
	static bool UsesHandle(const PassResourceDeclaration& declaration, FrameGraphTextureHandle handle) noexcept
	{
		return declaration.handle == handle.GetResourceHandle();
	}

	static std::wstring BuildHistoryResourceName(std::string_view name)
	{
		std::wstring result(name.begin(), name.end());
		result.append(L"History");
		return result;
	}
};

FrameGraphTextureHistory FrameGraph::CreateTextureHistory(const FrameGraphTextureDesc& desc) noexcept
{
	FrameGraphTextureDesc previousDesc = desc;
	previousDesc.name = "Previous" + desc.name + "History";
	FrameGraphTextureDesc currentDesc = desc;
	currentDesc.name = "Current" + desc.name + "History";

	const FrameGraphTextureHistory handles{
	    .Previous = ReservePersistentTexture(previousDesc, ResourceState::Undefined),
	    .Current = ReservePersistentTexture(currentDesc, ResourceState::Undefined)};
	m_textureHistories.push_back(TextureHistoryRecord{.handles = handles, .desc = desc});
	return handles;
}

void FrameGraph::InvalidateTextureHistory(FrameGraphTextureHistory history) noexcept
{
	const auto it = std::find_if(
	    m_textureHistories.begin(),
	    m_textureHistories.end(),
	    [history](const TextureHistoryRecord& record)
	    {
		    return record.handles.Previous == history.Previous && record.handles.Current == history.Current;
	    });
	if (it == m_textureHistories.end())
	{
		return;
	}

	++it->generation;
	if (it->generation == 0u)
	{
		++it->generation;
	}
}

bool FrameGraph::IsTextureHistoryValid(FrameGraphTextureHistory history) const noexcept
{
	const auto it = std::find_if(
	    m_textureHistories.begin(),
	    m_textureHistories.end(),
	    [history](const TextureHistoryRecord& record)
	    {
		    return record.handles.Previous == history.Previous && record.handles.Current == history.Current;
	    });
	return it != m_textureHistories.end() && it->usedThisFrame && it->generations[it->previousIndex] == it->generation;
}

void FrameGraph::PrepareTextureHistories(const FrameGraphPlan& plan)
{
	RhiResourceService& resourceService = m_renderHardwareInterface->GetResourceService();
	for (TextureHistoryRecord& history : m_textureHistories)
	{
		history.usedThisFrame = false;
		history.writtenThisFrame = false;
		bool requiresRenderTarget = false;
		bool requiresDepthStencil = history.desc.kind == FrameGraphTextureKind::DepthStencil;
		bool requiresUnorderedAccess = false;
		for (const FrameGraphPassNode& pass : plan.passes)
		{
			for (const PassResourceDeclaration& declaration : pass.declarations)
			{
				const bool usesPrevious = FrameGraphTextureHistoryOperations::UsesHandle(declaration, history.handles.Previous);
				const bool usesCurrent = FrameGraphTextureHistoryOperations::UsesHandle(declaration, history.handles.Current);
				if (!usesPrevious && !usesCurrent)
				{
					continue;
				}

				history.usedThisFrame = true;
				history.writtenThisFrame = history.writtenThisFrame || (usesCurrent && WritesToUsage(declaration.usage));
				requiresRenderTarget = requiresRenderTarget || declaration.usage == ResourceUsage::RenderTarget;
				requiresDepthStencil = requiresDepthStencil || declaration.usage == ResourceUsage::DepthWrite;
				requiresUnorderedAccess = requiresUnorderedAccess || declaration.usage == ResourceUsage::UnorderedAccess;
			}
		}

		if (!history.usedThisFrame)
		{
			continue;
		}

		const bool requiresRecreation =
		    (requiresRenderTarget && !history.allowRenderTarget) ||
		    (requiresDepthStencil && !history.allowDepthStencil) ||
		    (requiresUnorderedAccess && !history.allowUnorderedAccess);
		if (requiresRecreation && history.resources.front())
		{
			ReleaseExternalResourceViews(history.handles.Previous.GetResourceHandle());
			ReleaseExternalResourceViews(history.handles.Current.GetResourceHandle());
			for (RhiOwnedResourceHandle& resource : history.resources)
			{
				if (resource)
				{
					resourceService.ReleaseOwnedResource(resource);
					resource = {};
				}
			}
			history.states.fill(ResourceState::Undefined);
			history.generations.fill(0u);
			++history.generation;
			if (history.generation == 0u)
			{
				++history.generation;
			}
		}
		history.allowRenderTarget = history.allowRenderTarget || requiresRenderTarget;
		history.allowDepthStencil = history.allowDepthStencil || requiresDepthStencil;
		history.allowUnorderedAccess = history.allowUnorderedAccess || requiresUnorderedAccess;

		const FrameGraphTextureDesc& resolvedDesc =
		    m_resourceRegistry.GetMetadata(history.handles.Current.GetResourceHandle()).textureDesc;
		for (RhiOwnedResourceHandle& resource : history.resources)
		{
			if (!resource)
			{
				resource = resourceService.CreateTextureResource(
				    RhiTextureResourceDesc{
				        .Width = resolvedDesc.width,
				        .Height = resolvedDesc.height,
				        .Format = resolvedDesc.format,
				        .MipLevels = 1u,
				        .AllowRenderTarget = history.allowRenderTarget,
				        .AllowDepthStencil = history.allowDepthStencil,
				        .AllowUnorderedAccess = history.allowUnorderedAccess},
				    ResourceState::Undefined,
				    RhiMemoryCategory::Texture,
				    RhiMemoryResidencyClass::DeviceLocal,
				    FrameGraphTextureHistoryOperations::BuildHistoryResourceName(history.desc.name));
			}
		}

		history.currentIndex = m_renderHardwareInterface->GetCurrentFrameIndex() % RhiFrameConstants::FramesInFlight;
		history.previousIndex =
		    (history.currentIndex + RhiFrameConstants::FramesInFlight - 1u) % RhiFrameConstants::FramesInFlight;
		const ResourceState previousState = history.states[history.previousIndex];
		const ResourceState currentState = history.states[history.currentIndex];
		m_resourceRegistry.SetBoundaryStates(
		    history.handles.Previous.GetResourceHandle(), previousState, ResourceState::ShaderResource);
		m_resourceRegistry.SetBoundaryStates(
		    history.handles.Current.GetResourceHandle(), currentState, ResourceState::ShaderResource);
		BindPersistentTexture(history.handles.Previous, history.resources[history.previousIndex], previousState);
		BindPersistentTexture(history.handles.Current, history.resources[history.currentIndex], currentState);
	}
}

void FrameGraph::CommitTextureHistories() const noexcept
{
	for (TextureHistoryRecord& history : m_textureHistories)
	{
		if (!history.usedThisFrame)
		{
			continue;
		}

		history.states[history.previousIndex] = ResourceState::ShaderResource;
		history.states[history.currentIndex] = ResourceState::ShaderResource;
		if (history.writtenThisFrame)
		{
			history.generations[history.currentIndex] = history.generation;
		}
	}
}

void FrameGraph::ReleaseTextureHistories() noexcept
{
	if (m_renderHardwareInterface == nullptr)
	{
		return;
	}

	RhiResourceService& resourceService = m_renderHardwareInterface->GetResourceService();
	for (TextureHistoryRecord& history : m_textureHistories)
	{
		for (RhiOwnedResourceHandle& resource : history.resources)
		{
			if (resource)
			{
				resourceService.ReleaseOwnedResource(resource);
				resource = {};
			}
		}
	}
	m_textureHistories.clear();
}
