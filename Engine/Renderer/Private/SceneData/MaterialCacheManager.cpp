#include "PCH.h"

#include "MaterialCacheManager.h"

#include "GameFramework/Public/Assets/MaterialDesc.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12DescriptorHeapManager.h"
#include "D3D12RootBindings.h"
#include "D3D12Texture.h"
#include "Renderer/Public/SceneData/SceneView.h"
#include "Renderer/Public/Textures/MaterialFallbackTextures.h"
#include "SceneData/MaterialCacheUtils.h"
#include "SceneData/RenderSceneSnapshot.h"
#include "TextureManager.h"

MaterialCacheManager::MaterialCacheManager(TextureManager& textureManager, D3D12DescriptorHeapManager& descriptorHeapManager) noexcept :
	m_textureManager(&textureManager), m_descriptorHeapManager(&descriptorHeapManager)
{}

MaterialCacheManager::~MaterialCacheManager() noexcept
{
	Reset();
}

void MaterialCacheManager::PopulateSceneMaterials(const RenderSceneSnapshot& sceneSnapshot, SceneView& view)
{
	const auto& loadedMaterials = sceneSnapshot.materials;
	const bool shouldUseLoadedMaterials = !loadedMaterials.empty();
	const bool materialSetChanged = shouldUseLoadedMaterials
	                                    ? (!m_materialCacheUsesLoadedMaterials ||
	                                       !MaterialCacheUtils::MaterialDescSetEquals(m_cachedMaterialDescs, loadedMaterials))
	                                    : m_materialCacheUsesLoadedMaterials;

	if (!m_materialCacheBuilt || materialSetChanged)
	{
		Rebuild(sceneSnapshot);
	}

	if (!m_cachedMaterialData.empty())
	{
		view.materials = m_cachedMaterialData;
	}
}

void MaterialCacheManager::Rebuild(const RenderSceneSnapshot& sceneSnapshot)
{
	if (!m_textureManager || !m_descriptorHeapManager)
	{
		LOG_FATAL("MaterialCacheManager::Rebuild: required renderer dependencies are unavailable.");
		return;
	}

	const auto& loadedMaterials = sceneSnapshot.materials;

	ReleaseMaterialTextureTables();
	m_cachedMaterialData.clear();
	m_cachedMaterialDescs.clear();
	m_materialCacheBuilt = false;
	m_materialCacheUsesLoadedMaterials = !loadedMaterials.empty();

	const auto* srvHeap = m_descriptorHeapManager->GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	if (!srvHeap)
	{
		LOG_FATAL("MaterialCacheManager::Rebuild: SRV heap is unavailable.");
		return;
	}

	auto buildMaterialTable = [this, srvHeap](const MaterialDesc& desc)
	{
		MaterialData material = MaterialData::FromDesc(desc);

		const D3D12Texture* textures[RootBindings::SRVRegister::MaterialTextureCount] = {
		    MaterialCacheUtils::ResolveMaterialTexture(*m_textureManager, desc.albedoTexture, MaterialFallbackTexture::Albedo),
		    MaterialCacheUtils::ResolveMaterialTexture(*m_textureManager, desc.normalTexture, MaterialFallbackTexture::Normal),
		    MaterialCacheUtils::ResolveMaterialTexture(
		        *m_textureManager,
		        desc.metallicRoughnessTexture,
		        MaterialFallbackTexture::MetallicRoughness),
		    MaterialCacheUtils::ResolveMaterialTexture(*m_textureManager, desc.occlusionTexture, MaterialFallbackTexture::Occlusion),
		    MaterialCacheUtils::ResolveMaterialTexture(*m_textureManager, desc.emissiveTexture, MaterialFallbackTexture::Emissive)};

		const D3D12DescriptorHandle tableHandle = m_descriptorHeapManager->AllocateContiguous(
		    D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		    RootBindings::SRVRegister::MaterialTextureCount);

		for (std::uint32_t slot = 0; slot < RootBindings::SRVRegister::MaterialTextureCount; ++slot)
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

	if (!loadedMaterials.empty())
	{
		m_cachedMaterialDescs = loadedMaterials;
		m_cachedMaterialData.reserve(loadedMaterials.size());
		m_materialTextureTables.reserve(loadedMaterials.size());

		for (const auto& desc : loadedMaterials)
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
	m_cachedMaterialDescs.clear();
	m_materialCacheBuilt = false;
	m_materialCacheUsesLoadedMaterials = false;
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
			m_descriptorHeapManager->FreeContiguous(
			    D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			    tableHandle,
			    RootBindings::SRVRegister::MaterialTextureCount);
		}
	}

	m_materialTextureTables.clear();
}