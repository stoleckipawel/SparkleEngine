#include "PCH.h"
#include "Resources/History/PersistentTextureHistory.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/FrameGraph.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureDesc.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "RHI/Public/Interop/ResourceState.h"
#include "RHI/Public/Memory/RhiMemoryTypes.h"
#include "RHI/Public/Resources/RhiResourceDesc.h"
#include "RHI/Public/Resources/RhiResourceService.h"

PersistentTextureHistory::PersistentTextureHistory(
    RenderHardwareInterface& renderHardwareInterface,
    PersistentTextureHistorySpec spec) :
	PersistentTextureHistory(renderHardwareInterface, spec.Name, spec.Format)
{
}

PersistentTextureHistory::PersistentTextureHistory(
    RenderHardwareInterface& renderHardwareInterface,
    std::string_view name,
    PixelFormat format) :
	m_renderHardwareInterface(renderHardwareInterface),
	m_resourceName(name.begin(), name.end()),
	m_format(format)
{
	m_resourceName.append(L"History");
}

PersistentTextureHistory::~PersistentTextureHistory() noexcept
{
	Release();
}

FrameGraphTextureHistoryHandles PersistentTextureHistory::Reserve(
    FrameGraphBuilder& builder,
    RenderViewportExtent extent,
    const PersistentTextureHistorySpec& spec)
{
	return Reserve(builder, extent, spec.Name, spec.Format);
}

FrameGraphTextureHistoryHandles PersistentTextureHistory::Reserve(
    FrameGraphBuilder& builder,
    RenderViewportExtent extent,
    std::string_view name,
    PixelFormat format)
{
	std::string previousName = "Previous";
	previousName.append(name);
	previousName.append("History");
	std::string currentName = "Current";
	currentName.append(name);
	currentName.append("History");
	return FrameGraphTextureHistoryHandles{
	    .Previous = builder.ReservePersistentTexture(
	        FrameGraphTextureDesc::CreateColor(previousName, extent.Width, extent.Height, format),
	        ResourceState::ShaderResource),
	    .Current = builder.ReservePersistentTexture(
	        FrameGraphTextureDesc::CreateColor(currentName, extent.Width, extent.Height, format),
	        ResourceState::ShaderResource)};
}

void PersistentTextureHistory::SetGraphHandles(FrameGraphTextureHistoryHandles handles) noexcept
{
	m_graphHandles = handles;
	m_valid = false;
}

void PersistentTextureHistory::Configure(bool active, RenderViewportExtent extent)
{
	if (!active || !extent.IsValid())
	{
		m_active = false;
		m_semanticKey = 0;
		Release();
		return;
	}

	if (!m_active || extent != m_extent)
	{
		m_valid = false;
	}
	m_active = true;

	RhiResourceService& resourceService = m_renderHardwareInterface.GetResourceService();
	if (extent != m_extent)
	{
		Release();
	}
	m_extent = extent;

	const RhiTextureResourceDesc desc{
	    .Width = extent.Width,
	    .Height = extent.Height,
	    .Format = m_format,
	    .MipLevels = 1u,
	    .AllowUnorderedAccess = true};
	for (RhiOwnedResourceHandle& resource : m_resources)
	{
		if (!resource)
		{
			resource = resourceService.CreateTextureResource(
			    desc,
			    ResourceState::Undefined,
			    RhiMemoryCategory::Texture,
			    RhiMemoryResidencyClass::DeviceLocal,
			    m_resourceName);
			m_valid = false;
		}
	}
}

bool PersistentTextureHistory::SetSemanticKey(std::uint64_t key) noexcept
{
	const bool changed = m_semanticKey != 0 && m_semanticKey != key;
	m_semanticKey = key;
	if (changed)
	{
		m_valid = false;
	}
	return m_active && changed;
}

void PersistentTextureHistory::Bind(
    FrameGraph& frameGraph,
    std::uint32_t frameIndex,
    std::uint64_t resetGeneration) noexcept
{
	if (m_consumedResetGeneration != resetGeneration)
	{
		m_consumedResetGeneration = resetGeneration;
		m_valid = false;
	}

	if (!m_graphHandles.IsValid())
	{
		m_valid = false;
		return;
	}

	const std::uint32_t currentIndex = frameIndex % RhiFrameConstants::FramesInFlight;
	const std::uint32_t previousIndex =
	    (currentIndex + RhiFrameConstants::FramesInFlight - 1u) % RhiFrameConstants::FramesInFlight;
	const RhiOwnedResourceHandle previous = m_resources[previousIndex];
	const RhiOwnedResourceHandle current = m_resources[currentIndex];
	if (!m_active || !previous || !current)
	{
		frameGraph.ClearPersistentTextureBinding(m_graphHandles.Previous);
		frameGraph.ClearPersistentTextureBinding(m_graphHandles.Current);
		m_valid = false;
		return;
	}

	const ResourceState state = m_valid ? ResourceState::ShaderResource : ResourceState::Undefined;
	frameGraph.BindPersistentTexture(m_graphHandles.Previous, previous, state);
	frameGraph.BindPersistentTexture(m_graphHandles.Current, current, state);
}

void PersistentTextureHistory::CommitFrame() noexcept
{
	m_valid = m_active && m_graphHandles.IsValid() && HasResources();
}

void PersistentTextureHistory::Release() noexcept
{
	RhiResourceService& resourceService = m_renderHardwareInterface.GetResourceService();
	for (RhiOwnedResourceHandle& resource : m_resources)
	{
		if (resource)
		{
			resourceService.ReleaseOwnedResource(resource);
			resource = {};
		}
	}
	m_extent = {};
	m_valid = false;
}

bool PersistentTextureHistory::HasResources() const noexcept
{
	for (const RhiOwnedResourceHandle resource : m_resources)
	{
		if (!resource)
		{
			return false;
		}
	}
	return true;
}
