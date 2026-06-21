#include "PCH.h"
#include "SceneData/MaterialTextureTable.h"

#include "Resources/Texture.h"
#include "SceneData/MaterialTextureTableCapability.h"
#include "RHI/Public/Bindings/RenderBindingSet.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"

#include <limits>
#include <utility>

void MaterialTextureTable::Reset() noexcept
{
	m_bindingSet.reset();
	m_textures.clear();
	m_textureCount = 0u;
}

std::uint32_t MaterialTextureTable::GetOrAddTextureIndex(const Texture* texture)
{
	if (texture == nullptr)
	{
		return MaterialTextureInvalidIndex;
	}

	for (std::uint32_t index = 0u; index < static_cast<std::uint32_t>(m_textures.size()); ++index)
	{
		if (m_textures[index] == texture)
		{
			return index;
		}
	}

	if (m_textures.size() >= static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()))
	{
		return MaterialTextureInvalidIndex;
	}

	const std::uint32_t index = static_cast<std::uint32_t>(m_textures.size());
	m_textures.push_back(texture);
	return index;
}

MaterialTextureTableBuildResult MaterialTextureTable::BuildBindingSet(RenderHardwareInterface& renderHardwareInterface)
{
	m_bindingSet.reset();
	m_textureCount = 0u;
	if (m_textures.empty())
	{
		return MaterialTextureTableBuildResult{.FailureReason = "empty-texture-table"};
	}
	if (m_textures.size() > MaterialTextureTableFixedCapacity)
	{
		return MaterialTextureTableBuildResult{.FailureReason = "fixed-capacity-descriptor-array-overflow"};
	}

	for (const Texture* texture : m_textures)
	{
		if (texture == nullptr)
		{
			return MaterialTextureTableBuildResult{.FailureReason = "missing-texture-descriptor"};
		}
	}

	auto bindingSet = renderHardwareInterface.GetDescriptorService().CreateBindingSet(
	    RenderBindingSetDesc{
	        .DescriptorType = ERhiDescriptorAllocatorType::ShaderResource,
	        .DescriptorCount = static_cast<std::uint32_t>(m_textures.size())});
	if (!bindingSet || !*bindingSet)
	{
		return MaterialTextureTableBuildResult{.FailureReason = "descriptor-table-allocation-failed"};
	}

	for (std::uint32_t index = 0u; index < static_cast<std::uint32_t>(m_textures.size()); ++index)
	{
		m_textures[index]->WriteShaderResourceView(bindingSet->GetCpuDescriptorHandle(index));
	}

	m_textureCount = static_cast<std::uint32_t>(m_textures.size());
	m_bindingSet = std::move(bindingSet);
	return MaterialTextureTableBuildResult{
	    .Valid = true,
	    .FailureReason = "available",
	    .TextureCount = m_textureCount};
}
