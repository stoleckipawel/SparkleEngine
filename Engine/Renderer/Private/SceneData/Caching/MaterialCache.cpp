#include "PCH.h"

#include "MaterialCache.h"

#include "Scene/Materials/MaterialDesc.h"
#include "SceneData/MaterialData.h"
#include "SceneData/RenderSceneData.h"
#include "Renderer/Public/Resources/Textures/DefaultTextures.h"
#include "RHI/Public/Bindings/RenderBindingSet.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "SceneData/MaterialTextureTableCapability.h"
#include "Textures/TextureCache.h"
#include "Textures/RendererTexture.h"

#include <array>
#include <format>

static const auto g_materialCacheLogger = Logging::GetOrCreateLogger("Renderer.MaterialCache");

struct MaterialCache::RebuildOutput final
{
	std::vector<MaterialData> Materials;
	std::vector<std::unique_ptr<RenderBindingSet>> RasterTextureTables;
	MaterialTextureTable SceneTextureTable;
};

MaterialCache::MaterialCache(TextureCache& textureCache, RenderHardwareInterface& renderHardwareInterface) noexcept :
    m_textureCache(textureCache), m_renderHardwareInterface(renderHardwareInterface)
{
}

MaterialCache::~MaterialCache() noexcept
{
	Reset();
}

void MaterialCache::BuildMaterials(const RenderMaterialTable& materials, std::uint64_t sourceRevision, RenderSceneData& sceneData)
{
	const std::uint64_t textureRevision = m_textureCache.GetBindingRevision();
	const bool rebuildRequired = !m_materialCacheBuilt || m_sourceRevision != sourceRevision || m_textureRevision != textureRevision;
	if (rebuildRequired)
	{
		Rebuild(materials, sourceRevision, textureRevision);
	}

	sceneData.materials = m_cachedMaterialData;
	PublishMaterialTextureTable(sceneData);
}

void MaterialCache::Rebuild(const RenderMaterialTable& materials, std::uint64_t sourceRevision, std::uint64_t textureRevision)
{
	const std::uint64_t nextGeneration = GetNextGeneration();
	RebuildOutput output;
	const RendererTexture* tableAnchor =
	    m_textureCache.ResolveTextureReferenceOrSemanticDefault(nullptr, DefaultTexture::White);
	if (tableAnchor == nullptr)
	{
		Diagnostics::Fatal(
		    g_materialCacheLogger,
		    __FILE__,
		    __LINE__,
		    "Material texture table anchor was not loaded.");
	}
	output.SceneTextureTable.GetOrAddTextureIndex(tableAnchor->ShaderResourceView);

	if (!materials.Values.empty())
	{
		output.Materials.reserve(materials.Values.size());
		output.RasterTextureTables.reserve(materials.Values.size());

		for (std::uint32_t materialIndex = 0u; materialIndex < static_cast<std::uint32_t>(materials.Values.size()); ++materialIndex)
		{
			BuildMaterial(materials.Values[materialIndex], materialIndex, nextGeneration, output);
		}
	}
	output.SceneTextureTable.BuildBindingSet(m_renderHardwareInterface);

	m_cachedMaterialData = std::move(output.Materials);
	m_materialTextureBindingSets = std::move(output.RasterTextureTables);
	m_materialTextureTable = std::move(output.SceneTextureTable);
	m_sourceRevision = sourceRevision;
	m_textureRevision = textureRevision;
	m_generation = nextGeneration;
	m_materialCacheBuilt = true;
	m_textureCache.CommitBindingRevision(textureRevision);
}

void MaterialCache::BuildMaterial(
    const MaterialDesc& desc,
    std::uint32_t materialIndex,
    std::uint64_t generation,
    RebuildOutput& output)
{
	MaterialData material = MaterialData::FromDesc(desc);
	material.gpuHandle = MaterialGpuHandle{.Index = materialIndex, .Generation = generation};

	const std::array<const RendererTexture*, MaterialTextureSlots::Count> textures{
	    m_textureCache.ResolveTextureReferenceOrSemanticDefault(desc.FindTextureReference(TextureGroup::Diffuse), DefaultTexture::White),
	    m_textureCache.ResolveTextureReferenceOrSemanticDefault(
	        desc.FindTextureReference(TextureGroup::NormalMap),
	        DefaultTexture::Normal),
	    m_textureCache.ResolveTextureReferenceOrSemanticDefault(
	        desc.FindTextureReference(TextureGroup::Roughness),
	        DefaultTexture::White),
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
		Diagnostics::Fatal(
		    g_materialCacheLogger,
		    __FILE__,
		    __LINE__,
		    "Raster material texture-table allocation failed.");
	}

	for (std::uint32_t slot = 0u; slot < textures.size(); ++slot)
	{
		const RendererTexture* texture = textures[slot];
		if (texture == nullptr)
		{
			Diagnostics::Fatal(
			    g_materialCacheLogger,
			    __FILE__,
			    __LINE__,
			    "A semantic material texture was not loaded.");
		}

		const RhiResourceViewHandle textureView = texture->ShaderResourceView;
		if (!textureBindingSet->WriteResourceView(slot, textureView))
		{
			Diagnostics::Fatal(
			    g_materialCacheLogger,
			    __FILE__,
			    __LINE__,
			    "Raster material texture descriptor write failed.");
		}

		material.materialTextureIndices[slot] = output.SceneTextureTable.GetOrAddTextureIndex(textureView);
	}

	material.rasterTextureTable = textureBindingSet->GetTableBinding(0u);
	if (!material.rasterTextureTable)
	{
		Diagnostics::Fatal(
		    g_materialCacheLogger,
		    __FILE__,
		    __LINE__,
		    "Raster material texture table has no GPU binding.");
	}

	output.RasterTextureTables.push_back(std::move(textureBindingSet));
	output.Materials.push_back(material);
}

void MaterialCache::Reset() noexcept
{
	m_materialTextureBindingSets.clear();
	m_materialTextureTable.Reset();
	m_cachedMaterialData.clear();
	m_sourceRevision = 0u;
	m_textureRevision = 0u;
	m_materialCacheBuilt = false;
}

void MaterialCache::PublishMaterialTextureTable(RenderSceneData& sceneData) const noexcept
{
	const std::uint32_t descriptorCount = m_materialTextureTable.GetTextureCount();
	const RhiDescriptorTableBinding binding = m_materialTextureTable.GetTableBinding();
	if (!m_materialTextureTable.IsValid() || !binding || descriptorCount > MaterialTextureTableFixedCapacity)
	{
		Diagnostics::Fatal(
		    g_materialCacheLogger,
		    __FILE__,
		    __LINE__,
		    "Material texture table publication contract is incomplete.");
	}
	sceneData.materialTextureTable =
	    ResolvedMaterialTextureTable{.Binding = binding, .DescriptorCount = descriptorCount, .Generation = m_generation};
}

std::uint64_t MaterialCache::GetNextGeneration() const noexcept
{
	const std::uint64_t nextGeneration = m_generation + 1u;
	if (nextGeneration == 0u)
	{
		Diagnostics::Fatal(g_materialCacheLogger, __FILE__, __LINE__, "Material cache generation overflowed.");
	}
	return nextGeneration;
}
