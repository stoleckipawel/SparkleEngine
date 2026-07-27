#include "PCH.h"

#include "MaterialCacheManager.h"

#include "Scene/Materials/MaterialDesc.h"
#include "SceneData/MaterialData.h"
#include "SceneData/RenderSceneData.h"
#include "Renderer/Public/Resources/Textures/DefaultTextures.h"
#include "RHI/Public/Bindings/RenderBindingSet.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "SceneData/Caching/MaterialCacheUtils.h"
#include "SceneData/MaterialTextureTableCapability.h"
#include "Textures/TextureManager.h"
#include "Textures/RendererTexture.h"

#include <array>

static const auto g_materialCacheManagerLogger = Logging::GetOrCreateLogger("Renderer.MaterialCache");

struct MaterialCacheManager::Build final
{
	std::vector<MaterialData> Materials;
	std::vector<std::unique_ptr<RenderBindingSet>> RasterTextureTables;
	MaterialTextureTable SceneTextureTable;
};

MaterialCacheManager::MaterialCacheManager(
    TextureManager& textureManager,
    RenderHardwareInterface& renderHardwareInterface) noexcept :
	m_textureManager(textureManager),
	m_renderHardwareInterface(renderHardwareInterface)
{
}

MaterialCacheManager::~MaterialCacheManager() noexcept
{
	Reset();
}

void MaterialCacheManager::BuildMaterials(
    const RenderMaterialTable& materials,
    std::uint64_t sourceRevision,
    RenderSceneData& sceneData)
{
	const std::uint64_t textureRevision =
	    m_textureManager.GetBindingRevision();
	if ((!m_materialCacheBuilt ||
	     m_sourceRevision != sourceRevision ||
	     m_textureRevision != textureRevision) &&
	    !Rebuild(
	        materials,
	        sourceRevision,
	        textureRevision) &&
	    !m_materialCacheBuilt)
	{
		sceneData.materials = {};
		sceneData.materialTextureTable = {};
		return;
	}

	sceneData.materials = m_cachedMaterialData;
	PublishMaterialTextureTable(sceneData);
}

bool MaterialCacheManager::Rebuild(
    const RenderMaterialTable& materials,
    std::uint64_t sourceRevision,
    std::uint64_t textureRevision)
{
	const std::uint64_t nextGeneration = GetNextGeneration();
	Build build;

	if (!materials.Values.empty())
	{
		build.Materials.reserve(materials.Values.size());
		build.RasterTextureTables.reserve(materials.Values.size());

		for (std::uint32_t materialIndex = 0u;
		     materialIndex < static_cast<std::uint32_t>(materials.Values.size());
		     ++materialIndex)
		{
			if (!BuildMaterial(
			        materials.Values[materialIndex],
			        materialIndex,
			        nextGeneration,
			        build))
			{
				return false;
			}
		}
	}
	else
	{
		build.Materials.reserve(1u);
		build.RasterTextureTables.reserve(1u);

		MaterialDesc defaultMaterial;
		defaultMaterial.name = "Renderer_DefaultMaterial";
		if (!BuildMaterial(defaultMaterial, 0u, nextGeneration, build))
		{
			return false;
		}
	}

	const MaterialTextureTableBuildResult rebuiltSceneTableResult =
	    build.SceneTextureTable.BuildBindingSet(m_renderHardwareInterface);
	if (!rebuiltSceneTableResult.Valid)
	{
		SPDLOG_LOGGER_WARN(
		    g_materialCacheManagerLogger,
		    "MaterialCacheManager::Rebuild: material texture table unavailable: reason={}.",
		    rebuiltSceneTableResult.FailureReason);
	}

	m_cachedMaterialData = std::move(build.Materials);
	m_materialTextureBindingSets = std::move(build.RasterTextureTables);
	m_materialTextureTable = std::move(build.SceneTextureTable);
	m_sourceRevision = sourceRevision;
	m_textureRevision = textureRevision;
	m_generation = nextGeneration;
	m_materialCacheBuilt = true;
	m_textureManager.CommitBindingRevision(
	    textureRevision);
	return true;
}

bool MaterialCacheManager::BuildMaterial(
    const MaterialDesc& desc,
    std::uint32_t materialIndex,
    std::uint64_t generation,
    Build& build)
{
	MaterialData material = MaterialData::FromDesc(desc);
	material.gpuHandle = MaterialGpuHandle{
	    .Index = materialIndex,
	    .Generation = generation};

	const std::array<const RendererTexture*, MaterialTextureSlots::Count> textures{
	    m_textureManager.ResolveTextureReferenceOrDefault(
	        desc.FindTextureReference(TextureGroup::Diffuse),
	        DefaultTexture::White),
	    m_textureManager.ResolveTextureReferenceOrDefault(
	        desc.FindTextureReference(TextureGroup::NormalMap),
	        DefaultTexture::Normal),
	    m_textureManager.ResolveTextureReferenceOrDefault(
	        desc.FindTextureReference(TextureGroup::Roughness),
	        DefaultTexture::White),
	    m_textureManager.ResolveTextureReferenceOrDefault(
	        desc.FindTextureReference(TextureGroup::Metallic),
	        DefaultTexture::Black),
	    m_textureManager.ResolveTextureReferenceOrDefault(
	        desc.FindTextureReference(TextureGroup::AmbientOcclusion),
	        DefaultTexture::White),
	    m_textureManager.ResolveTextureReferenceOrDefault(
	        desc.FindTextureReference(TextureGroup::Emissive),
	        DefaultTexture::Black),
	    m_textureManager.ResolveTextureReferenceOrDefault(
	        desc.FindTextureReference(TextureGroup::SubsurfaceColor),
	        DefaultTexture::Black),
	    m_textureManager.ResolveTextureReferenceOrDefault(
	        desc.FindTextureReference(TextureGroup::SubsurfaceStrength),
	        DefaultTexture::Black)};

	auto textureBindingSet =
	    m_renderHardwareInterface.GetDescriptorService().CreateBindingSet(
	        RenderBindingSetDesc{
	            .DescriptorType = ERhiDescriptorAllocatorType::ShaderResource,
	            .DescriptorCount = MaterialTextureSlots::Count});
	if (!textureBindingSet || !*textureBindingSet)
	{
		return false;
	}

	for (std::uint32_t slot = 0u; slot < textures.size(); ++slot)
	{
		const RendererTexture* texture = textures[slot];
		if (texture == nullptr)
		{
			return false;
		}

		const RhiResourceViewHandle textureView = texture->ShaderResourceView;
		if (!textureBindingSet->WriteResourceView(slot, textureView))
		{
			return false;
		}

		material.materialTextureIndices[slot] =
		    build.SceneTextureTable.GetOrAddTextureIndex(textureView);
	}

	material.rasterTextureTable = textureBindingSet->GetTableBinding(0u);
	if (!material.rasterTextureTable)
	{
		return false;
	}

	build.RasterTextureTables.push_back(std::move(textureBindingSet));
	build.Materials.push_back(material);
	return true;
}

void MaterialCacheManager::Reset() noexcept
{
	m_materialTextureBindingSets.clear();
	m_materialTextureTable.Reset();
	m_cachedMaterialData.clear();
	m_sourceRevision = 0u;
	m_textureRevision = 0u;
	m_materialCacheBuilt = false;
}

void MaterialCacheManager::PublishMaterialTextureTable(RenderSceneData& sceneData) const noexcept
{
	const std::uint32_t descriptorCount = m_materialTextureTable.GetTextureCount();
	const RhiDescriptorTableBinding binding = m_materialTextureTable.GetTableBinding();
	const bool ready = m_materialTextureTable.IsValid() && binding && descriptorCount <= MaterialTextureTableFixedCapacity;
	sceneData.materialTextureTable = ready ? ResolvedMaterialTextureTable{
	                                                 .Binding = binding,
	                                                 .DescriptorCount = descriptorCount,
	                                                 .Generation = m_generation}
	                                           : ResolvedMaterialTextureTable{};
}

std::uint64_t MaterialCacheManager::GetNextGeneration() const noexcept
{
	const std::uint64_t nextGeneration = m_generation + 1u;
	return nextGeneration != 0u ? nextGeneration : 1u;
}
