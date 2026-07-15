#include "PCH.h"
#include "SceneData/MaterialTextureTable.h"

#include "SceneData/MaterialTextureTableCapability.h"
#include "RHI/Public/Bindings/RenderBindingSet.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"

#include <limits>
#include <utility>

void MaterialTextureTable::Reset() noexcept
{
	m_bindingSet.reset();
	m_textureViews.clear();
}

std::uint32_t MaterialTextureTable::GetOrAddTextureIndex(RhiResourceViewHandle textureView)
{
	if (!textureView)
	{
		return MaterialTextureInvalidIndex;
	}

	for (std::uint32_t index = 0u; index < static_cast<std::uint32_t>(m_textureViews.size()); ++index)
	{
		if (m_textureViews[index] == textureView)
		{
			return index;
		}
	}

	if (m_textureViews.size() >= static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()))
	{
		return MaterialTextureInvalidIndex;
	}

	const std::uint32_t index = static_cast<std::uint32_t>(m_textureViews.size());
	m_textureViews.push_back(textureView);
	return index;
}

MaterialTextureTableBuildResult MaterialTextureTable::BuildBindingSet(RenderHardwareInterface& renderHardwareInterface)
{
	m_bindingSet.reset();
	if (m_textureViews.empty())
	{
		return MaterialTextureTableBuildResult{.FailureReason = "empty-texture-table"};
	}
	if (m_textureViews.size() > MaterialTextureTableFixedCapacity)
	{
		return MaterialTextureTableBuildResult{.FailureReason = "fixed-capacity-descriptor-array-overflow"};
	}

	for (const RhiResourceViewHandle textureView : m_textureViews)
	{
		if (!textureView)
		{
			return MaterialTextureTableBuildResult{.FailureReason = "missing-texture-descriptor"};
		}
	}

	auto bindingSet = renderHardwareInterface.GetDescriptorService().CreateBindingSet(
	    RenderBindingSetDesc{
	        .DescriptorType = ERhiDescriptorAllocatorType::ShaderResource,
	        .DescriptorCount = static_cast<std::uint32_t>(m_textureViews.size())});
	if (!bindingSet || !*bindingSet)
	{
		return MaterialTextureTableBuildResult{.FailureReason = "descriptor-table-allocation-failed"};
	}

	for (std::uint32_t index = 0u; index < static_cast<std::uint32_t>(m_textureViews.size()); ++index)
	{
		if (!bindingSet->WriteResourceView(index, m_textureViews[index]))
		{
			return MaterialTextureTableBuildResult{.FailureReason = "resource-view-write-failed"};
		}
	}

	m_bindingSet = std::move(bindingSet);
	return MaterialTextureTableBuildResult{
	    .Valid = true,
	    .FailureReason = "available"};
}

RhiDescriptorTableBinding MaterialTextureTable::GetTableBinding() const noexcept
{
	return m_bindingSet != nullptr ? m_bindingSet->GetTableBinding(0u) : RhiDescriptorTableBinding{};
}
