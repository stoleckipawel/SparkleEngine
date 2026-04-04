#include "PCH.h"

#include "MaterialCacheManager.h"

#include "GameFramework/Public/Assets/Import/MaterialDesc.h"
#include "GameFramework/Public/Scene/Materials/MaterialSnapshot.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12DescriptorHeapManager.h"
#include "D3D12Texture.h"
#include "Renderer/Public/SceneData/MaterialData.h"
#include "Renderer/Public/SceneData/RenderSceneData.h"
#include "Renderer/Public/Textures/DefaultTextures.h"
#include "SceneData/MaterialCacheUtils.h"
#include "Renderer/Public/Textures/TextureManager.h"

MaterialCacheManager::MaterialCacheManager(TextureManager& textureManager, D3D12DescriptorHeapManager& descriptorHeapManager) noexcept :
    m_textureManager(&textureManager), m_descriptorHeapManager(&descriptorHeapManager)
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
		Rebuild(materialSnapshot);
	}

	if (!m_cachedMaterialData.empty())
	{
		sceneData.materials = m_cachedMaterialData;
	}
}

void MaterialCacheManager::Rebuild(const MaterialSnapshot& materialSnapshot)
{
	if (!m_textureManager || !m_descriptorHeapManager)
	{
		LOG_FATAL("MaterialCacheManager::Rebuild: required renderer dependencies are unavailable.");
		return;
	}

	ReleaseMaterialTextureTables();
	m_cachedMaterialData.clear();
	m_cachedMaterialSnapshot.Reset();
	m_materialCacheBuilt = false;
	m_cachedFromSceneMaterials = materialSnapshot.HasMaterials();

	const auto* srvHeap = m_descriptorHeapManager->GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	if (!srvHeap)
	{
		LOG_FATAL("MaterialCacheManager::Rebuild: SRV heap is unavailable.");
		return;
	}

	auto buildMaterialTable = [this, srvHeap](const MaterialDesc& desc)
	{
		MaterialData material = MaterialData::FromDesc(desc);

		const D3D12Texture* textures[MaterialTextureSlots::Count] = {
		    m_textureManager->ResolveTextureOrDefault(desc.albedoTexture, DefaultTexture::White),
		    m_textureManager->ResolveTextureOrDefault(desc.normalTexture, DefaultTexture::Normal),
		    m_textureManager->ResolveTextureOrDefault(desc.metallicRoughnessTexture, DefaultTexture::Black),
		    m_textureManager->ResolveTextureOrDefault(desc.occlusionTexture, DefaultTexture::White),
		    m_textureManager->ResolveTextureOrDefault(desc.emissiveTexture, DefaultTexture::Black)};

		const D3D12DescriptorHandle tableHandle =
		    m_descriptorHeapManager->AllocateContiguous(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, MaterialTextureSlots::Count);

		for (std::uint32_t slot = 0; slot < MaterialTextureSlots::Count; ++slot)
		{
			if (!textures[slot])
			{
				LOG_FATAL(std::format("MaterialCacheManager::Rebuild: Material texture slot {} resolved to null.", slot));
			}

			const D3D12_CPU_DESCRIPTOR_HANDLE destination = srvHeap->GetHandleAt(tableHandle.GetIndex() + slot).GetCPU();
			textures[slot]->WriteShaderResourceView(destination);
		}

		material.textureTableGpuHandle = tableHandle.GetGPU();
		m_materialTextureTables.push_back(tableHandle);
		m_cachedMaterialData.push_back(material);
	};

	if (materialSnapshot.HasMaterials())
	{
		m_cachedMaterialSnapshot = materialSnapshot;
		m_cachedMaterialData.reserve(materialSnapshot.materialDescs.size());
		m_materialTextureTables.reserve(materialSnapshot.materialDescs.size());

		for (const auto& desc : materialSnapshot.materialDescs)
		{
			buildMaterialTable(desc);
		}
	}
	else
	{
		m_cachedMaterialData.reserve(1);
		m_materialTextureTables.reserve(1);

		MaterialDesc defaultMaterial;
		defaultMaterial.name = "Renderer_DefaultMaterial";
		buildMaterialTable(defaultMaterial);
	}

	m_materialCacheBuilt = true;
}

void MaterialCacheManager::Reset() noexcept
{
	ReleaseMaterialTextureTables();
	m_cachedMaterialData.clear();
	m_cachedMaterialSnapshot.Reset();
	m_materialCacheBuilt = false;
	m_cachedFromSceneMaterials = false;
}

void MaterialCacheManager::ReleaseMaterialTextureTables() noexcept
{
	if (!m_descriptorHeapManager)
	{
		return;
	}

	for (const D3D12DescriptorHandle& tableHandle : m_materialTextureTables)
	{
		if (tableHandle.IsValid())
		{
			m_descriptorHeapManager->FreeContiguous(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, tableHandle, MaterialTextureSlots::Count);
		}
	}

	m_materialTextureTables.clear();
}