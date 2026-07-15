#include "PCH.h"

#include "MaterialCacheManager.h"

#include "Scene/Materials/MaterialDesc.h"
#include "Scene/Materials/MaterialSnapshot.h"
#include "SceneData/MaterialData.h"
#include "SceneData/RenderSceneData.h"
#include "Renderer/Public/Resources/Textures/DefaultTextures.h"
#include "RHI/Public/Bindings/RenderBindingSet.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "SceneData/Caching/MaterialCacheUtils.h"
#include "SceneData/MaterialTextureTableCapability.h"
#include "Textures/TextureManager.h"
#include "Textures/RendererTexture.h"

static const auto g_materialCacheManagerLogger = Logging::GetOrCreateLogger("Renderer.MaterialCache");

MaterialCacheManager::MaterialCacheManager(TextureManager& textureManager, RenderHardwareInterface& renderHardwareInterface) noexcept :
	m_textureManager(&textureManager), m_renderHardwareInterface(&renderHardwareInterface)
{
}

MaterialCacheManager::~MaterialCacheManager() noexcept
{
	Reset();
}

void MaterialCacheManager::BuildMaterials(const MaterialSnapshot& materialSnapshot, RenderSceneData& sceneData)
{
	const bool shouldUseSceneMaterials = materialSnapshot.HasMaterials();
	const bool materialSetChanged =
	    shouldUseSceneMaterials
	        ? (!m_cachedFromSceneMaterials || !MaterialCacheUtils::MaterialSnapshotEquals(m_cachedMaterialSnapshot, materialSnapshot))
	        : m_cachedFromSceneMaterials;

	if (!m_materialCacheBuilt || materialSetChanged)
	{
		if (!Rebuild(materialSnapshot))
		{
			sceneData.materials.clear();
			sceneData.materialTextureTable = {};
			return;
		}
	}

	sceneData.materials = m_cachedMaterialData;
	PublishMaterialTextureTable(sceneData);
}

bool MaterialCacheManager::Rebuild(const MaterialSnapshot& materialSnapshot)
{
	if (!m_textureManager || !m_renderHardwareInterface)
	{
		Diagnostics::Fail(
		    g_materialCacheManagerLogger,
		    __FILE__,
		    __LINE__,
		    "MaterialCacheManager::Rebuild: required renderer dependencies are unavailable.");
		return false;
	}

	const std::uint64_t nextGeneration = GetNextGeneration();
	std::vector<MaterialData> rebuiltMaterialData;
	std::vector<std::unique_ptr<RenderBindingSet>> rebuiltRasterTextureTables;
	MaterialTextureTable rebuiltSceneTextureTable;

	auto buildMaterialTable =
	    [this, nextGeneration, &rebuiltMaterialData, &rebuiltRasterTextureTables, &rebuiltSceneTextureTable](
	        const MaterialDesc& desc,
	        std::uint32_t materialIndex) -> bool
	{
		MaterialData material = MaterialData::FromDesc(desc);
		material.gpuHandle = MaterialGpuHandle{.Index = materialIndex, .Generation = nextGeneration};

		const RendererTexture* textures[MaterialTextureSlots::Count] = {
		    m_textureManager->ResolveTextureReferenceOrDefault(desc.FindTextureReference(TextureGroup::Diffuse), DefaultTexture::White),
		    m_textureManager->ResolveTextureReferenceOrDefault(desc.FindTextureReference(TextureGroup::NormalMap), DefaultTexture::Normal),
		    m_textureManager->ResolveTextureReferenceOrDefault(desc.FindTextureReference(TextureGroup::Roughness), DefaultTexture::White),
		    m_textureManager->ResolveTextureReferenceOrDefault(desc.FindTextureReference(TextureGroup::Metallic), DefaultTexture::Black),
		    m_textureManager->ResolveTextureReferenceOrDefault(desc.FindTextureReference(TextureGroup::AmbientOcclusion), DefaultTexture::White),
		    m_textureManager->ResolveTextureReferenceOrDefault(desc.FindTextureReference(TextureGroup::Emissive), DefaultTexture::Black),
		    m_textureManager->ResolveTextureReferenceOrDefault(desc.FindTextureReference(TextureGroup::SubsurfaceColor), DefaultTexture::Black),
		    m_textureManager->ResolveTextureReferenceOrDefault(desc.FindTextureReference(TextureGroup::SubsurfaceStrength), DefaultTexture::Black)};

		auto textureBindingSet = m_renderHardwareInterface->GetDescriptorService().CreateBindingSet(
		    RenderBindingSetDesc{
		        .DescriptorType = ERhiDescriptorAllocatorType::ShaderResource,
		        .DescriptorCount = MaterialTextureSlots::Count});
		if (!textureBindingSet || !*textureBindingSet)
		{
			Diagnostics::Fail(
			    g_materialCacheManagerLogger,
			    __FILE__,
			    __LINE__,
			    "MaterialCacheManager::Rebuild: failed to allocate material texture binding set.");
			return false;
		}

		for (std::uint32_t slot = 0; slot < MaterialTextureSlots::Count; ++slot)
		{
			if (!textures[slot])
			{
				Diagnostics::Fail(
				    g_materialCacheManagerLogger,
				    __FILE__,
				    __LINE__,
				    std::format("MaterialCacheManager::Rebuild: Material texture slot {} resolved to null.", slot));
				return false;
			}

			const RhiResourceViewHandle textureView = textures[slot]->ShaderResourceView;
			if (!textureBindingSet->WriteResourceView(slot, textureView))
			{
				return false;
			}
			material.materialTextureIndices[slot] = rebuiltSceneTextureTable.GetOrAddTextureIndex(textureView);
		}

		material.rasterTextureTable = textureBindingSet->GetTableBinding(0u);
		if (!material.rasterTextureTable)
		{
			return false;
		}
		rebuiltRasterTextureTables.push_back(std::move(textureBindingSet));
		rebuiltMaterialData.push_back(material);
		return true;
	};

	if (materialSnapshot.HasMaterials())
	{
		rebuiltMaterialData.reserve(materialSnapshot.materialDescs.size());
		rebuiltRasterTextureTables.reserve(materialSnapshot.materialDescs.size());

		for (std::uint32_t materialIndex = 0u;
		     materialIndex < static_cast<std::uint32_t>(materialSnapshot.materialDescs.size());
		     ++materialIndex)
		{
			if (!buildMaterialTable(materialSnapshot.materialDescs[materialIndex], materialIndex))
			{
				return false;
			}
		}
	}
	else
	{
		rebuiltMaterialData.reserve(1);
		rebuiltRasterTextureTables.reserve(1);

		MaterialDesc defaultMaterial;
		defaultMaterial.name = "Renderer_DefaultMaterial";
		if (!buildMaterialTable(defaultMaterial, 0u))
		{
			return false;
		}
	}

	const MaterialTextureTableBuildResult rebuiltSceneTableResult =
	    rebuiltSceneTextureTable.BuildBindingSet(*m_renderHardwareInterface);
	if (!rebuiltSceneTableResult.Valid)
	{
		SPDLOG_LOGGER_WARN(
		    g_materialCacheManagerLogger,
		    "MaterialCacheManager::Rebuild: material texture table unavailable: reason={}.",
		    rebuiltSceneTableResult.FailureReason);
	}

	m_cachedMaterialSnapshot = materialSnapshot;
	m_cachedMaterialData = std::move(rebuiltMaterialData);
	// Replacing the owning binding sets releases the old descriptor tables through the RHI's per-frame retirement path.
	m_materialTextureBindingSets = std::move(rebuiltRasterTextureTables);
	m_materialTextureTable = std::move(rebuiltSceneTextureTable);
	m_generation = nextGeneration;
	m_materialCacheBuilt = true;
	m_cachedFromSceneMaterials = materialSnapshot.HasMaterials();
	return true;
}

void MaterialCacheManager::Reset() noexcept
{
	m_materialTextureBindingSets.clear();
	m_materialTextureTable.Reset();
	m_cachedMaterialData.clear();
	m_cachedMaterialSnapshot.Reset();
	m_materialCacheBuilt = false;
	m_cachedFromSceneMaterials = false;
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
