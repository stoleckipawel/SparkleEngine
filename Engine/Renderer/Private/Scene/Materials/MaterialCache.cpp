#include "PCH.h"

#include "Scene/Materials/MaterialCache.h"

#include "Renderer/Public/Resources/Textures/DefaultTextures.h"
#include "RHI/Public/Bindings/RenderBindingSet.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "Scene/Materials/MaterialData.h"
#include "Scene/Materials/MaterialDesc.h"
#include "Scene/Materials/MaterialTextureTableCapability.h"
#include "Scene/Materials/RenderMaterialGeneration.h"
#include "Scene/Preparation/PreparedRenderScene.h"
#include "Textures/RendererTexture.h"
#include "Textures/TextureCache.h"

#include <array>
#include <format>

static const auto g_materialCacheLogger = Logging::GetOrCreateLogger("Renderer.MaterialCache");

MaterialCache::MaterialCache(TextureCache& textureCache, RenderHardwareInterface& renderHardwareInterface) noexcept :
    m_textureCache(textureCache),
    m_renderHardwareInterface(renderHardwareInterface)
{
}

MaterialCache::~MaterialCache() noexcept = default;

void MaterialCache::BuildMaterials(const RenderMaterialTable& materials, std::uint64_t sourceRevision, PreparedRenderScene& preparedScene)
{
	const std::uint64_t textureRevision = m_textureCache.GetBindingRevision();
	const bool rebuildRequired = m_currentGeneration == nullptr || m_currentGeneration->GetSourceRevision() != sourceRevision
	    || m_currentGeneration->GetTextureRevision() != textureRevision;
	if (rebuildRequired)
	{
		Rebuild(materials, sourceRevision, textureRevision);
	}

	preparedScene.materialGeneration = m_currentGeneration;
	preparedScene.materials = m_currentGeneration->GetMaterials();
	PublishMaterialTextureTable(preparedScene);
}

void MaterialCache::Rebuild(const RenderMaterialTable& materials, std::uint64_t sourceRevision, std::uint64_t textureRevision)
{
	const std::uint64_t nextGeneration = GetNextGeneration();
	std::shared_ptr<RenderMaterialGeneration> output(new RenderMaterialGeneration());
	const RendererTexture* tableAnchor = m_textureCache.ResolveTextureReferenceOrSemanticDefault(nullptr, DefaultTexture::White);
	if (tableAnchor == nullptr)
	{
		Diagnostics::Fatal(g_materialCacheLogger, __FILE__, __LINE__, "Material texture table anchor was not loaded.");
	}
	output->m_textureTable.GetOrAddTextureIndex(tableAnchor->ShaderResourceView);

	if (!materials.Values.empty())
	{
		output->m_materials.reserve(materials.Values.size());
		output->m_rasterTextureTables.reserve(materials.Values.size());
		for (std::uint32_t materialIndex = 0u; materialIndex < static_cast<std::uint32_t>(materials.Values.size()); ++materialIndex)
		{
			BuildMaterial(materials.Values[materialIndex], materialIndex, nextGeneration, *output);
		}
	}
	output->m_textureTable.BuildBindingSet(m_renderHardwareInterface);
	output->m_sourceRevision = sourceRevision;
	output->m_textureRevision = textureRevision;
	output->m_generation = nextGeneration;
	m_currentGeneration = std::move(output);
	m_textureCache.CommitBindingRevision(textureRevision);
}

void MaterialCache::BuildMaterial(
    const MaterialDesc& desc,
    std::uint32_t materialIndex,
    std::uint64_t generation,
    RenderMaterialGeneration& output)
{
	MaterialData material = MaterialData::FromDesc(desc);
	material.gpuHandle = MaterialGpuHandle{.Index = materialIndex, .Generation = generation};

	const std::array<const RendererTexture*, MaterialTextureSlots::Count> textures{
	    m_textureCache.ResolveTextureReferenceOrSemanticDefault(desc.FindTextureReference(TextureGroup::Diffuse), DefaultTexture::White),
	    m_textureCache.ResolveTextureReferenceOrSemanticDefault(desc.FindTextureReference(TextureGroup::NormalMap), DefaultTexture::Normal),
	    m_textureCache.ResolveTextureReferenceOrSemanticDefault(desc.FindTextureReference(TextureGroup::Roughness), DefaultTexture::White),
	    m_textureCache.ResolveTextureReferenceOrSemanticDefault(desc.FindTextureReference(TextureGroup::Metallic), DefaultTexture::Black),
	    m_textureCache.ResolveTextureReferenceOrSemanticDefault(
	        desc.FindTextureReference(TextureGroup::AmbientOcclusion),
	        DefaultTexture::White),
	    m_textureCache.ResolveTextureReferenceOrSemanticDefault(desc.FindTextureReference(TextureGroup::Emissive), DefaultTexture::Black),
	    m_textureCache.ResolveTextureReferenceOrSemanticDefault(
	        desc.FindTextureReference(TextureGroup::SubsurfaceColor),
	        DefaultTexture::Black),
	    m_textureCache.ResolveTextureReferenceOrSemanticDefault(
	        desc.FindTextureReference(TextureGroup::SubsurfaceStrength),
	        DefaultTexture::Black)};

	auto textureBindingSet = m_renderHardwareInterface.GetDescriptorService().CreateBindingSet(
	    RenderBindingSetDesc{
	        .DescriptorType = ERhiDescriptorAllocatorType::ShaderResource,
	        .DescriptorCount = MaterialTextureSlots::Count});
	if (!textureBindingSet || !*textureBindingSet)
	{
		Diagnostics::Fatal(g_materialCacheLogger, __FILE__, __LINE__, "Raster material texture-table allocation failed.");
	}

	for (std::uint32_t slot = 0u; slot < textures.size(); ++slot)
	{
		const RendererTexture* texture = textures[slot];
		if (texture == nullptr)
		{
			Diagnostics::Fatal(g_materialCacheLogger, __FILE__, __LINE__, "A semantic material texture was not loaded.");
		}

		const RhiResourceViewHandle textureView = texture->ShaderResourceView;
		if (!textureBindingSet->WriteResourceView(slot, textureView))
		{
			Diagnostics::Fatal(g_materialCacheLogger, __FILE__, __LINE__, "Raster material texture descriptor write failed.");
		}
		material.materialTextureIndices[slot] = output.m_textureTable.GetOrAddTextureIndex(textureView);
	}

	material.rasterTextureTable = textureBindingSet->GetTableBinding(0u);
	if (!material.rasterTextureTable)
	{
		Diagnostics::Fatal(g_materialCacheLogger, __FILE__, __LINE__, "Raster material texture table has no GPU binding.");
	}

	output.m_rasterTextureTables.push_back(std::move(textureBindingSet));
	output.m_materials.push_back(material);
}

void MaterialCache::Reset() noexcept
{
	m_currentGeneration.reset();
}

void MaterialCache::PublishMaterialTextureTable(PreparedRenderScene& preparedScene) const noexcept
{
	const MaterialTextureTable& textureTable = m_currentGeneration->GetTextureTable();
	const std::uint32_t descriptorCount = textureTable.GetTextureCount();
	const RhiDescriptorTableBinding binding = textureTable.GetTableBinding();
	if (!textureTable.IsValid() || !binding || descriptorCount > MaterialTextureTableFixedCapacity)
	{
		Diagnostics::Fatal(g_materialCacheLogger, __FILE__, __LINE__, "Material texture table publication contract is incomplete.");
	}
	preparedScene.materialTextureTable = ResolvedMaterialTextureTable{
	    .Binding = binding,
	    .DescriptorCount = descriptorCount,
	    .Generation = m_currentGeneration->GetGeneration()};
}

std::uint64_t MaterialCache::GetNextGeneration() const noexcept
{
	const std::uint64_t currentGeneration = m_currentGeneration != nullptr ? m_currentGeneration->GetGeneration() : 0u;
	const std::uint64_t nextGeneration = currentGeneration + 1u;
	if (nextGeneration == 0u)
	{
		Diagnostics::Fatal(g_materialCacheLogger, __FILE__, __LINE__, "Material cache generation overflowed.");
	}
	return nextGeneration;
}
