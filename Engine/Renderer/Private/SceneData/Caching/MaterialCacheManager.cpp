#include "PCH.h"

#include "MaterialCacheManager.h"

#include "Scene/Materials/MaterialDesc.h"
#include "Scene/Materials/MaterialSnapshot.h"
#include "Resources/Texture.h"
#include "SceneData/MaterialData.h"
#include "SceneData/RenderSceneData.h"
#include "Renderer/Public/Resources/Textures/DefaultTextures.h"
#include "RHI/Public/Bindings/RenderBindingSet.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "SceneData/Caching/MaterialCacheUtils.h"
#include "Textures/TextureManager.h"

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
		Rebuild(materialSnapshot);
	}

	if (!m_cachedMaterialData.empty())
	{
		sceneData.materials = m_cachedMaterialData;
	}
}

void MaterialCacheManager::Rebuild(const MaterialSnapshot& materialSnapshot)
{
	if (!m_textureManager || !m_renderHardwareInterface)
	{
		Diagnostics::Fail(
		    g_materialCacheManagerLogger,
		    __FILE__,
		    __LINE__,
		    "MaterialCacheManager::Rebuild: required renderer dependencies are unavailable.");
		return;
	}

	ReleaseMaterialTextureBindingSets();
	m_cachedMaterialData.clear();
	m_cachedMaterialSnapshot.Reset();
	m_materialCacheBuilt = false;
	m_cachedFromSceneMaterials = materialSnapshot.HasMaterials();

	auto buildMaterialTable = [this](const MaterialDesc& desc)
	{
		MaterialData material = MaterialData::FromDesc(desc);

		const Texture* textures[MaterialTextureSlots::Count] = {
		    m_textureManager->ResolveTextureReferenceOrDefault(desc.FindTextureReference(TextureGroup::Diffuse), DefaultTexture::White),
		    m_textureManager->ResolveTextureReferenceOrDefault(desc.FindTextureReference(TextureGroup::NormalMap), DefaultTexture::Normal),
		    m_textureManager->ResolveTextureReferenceOrDefault(desc.FindTextureReference(TextureGroup::Roughness), DefaultTexture::White),
		    m_textureManager->ResolveTextureReferenceOrDefault(desc.FindTextureReference(TextureGroup::Metallic), DefaultTexture::Black),
		    m_textureManager->ResolveTextureReferenceOrDefault(desc.FindTextureReference(TextureGroup::AmbientOcclusion), DefaultTexture::White),
		    m_textureManager->ResolveTextureReferenceOrDefault(desc.FindTextureReference(TextureGroup::Emissive), DefaultTexture::Black),
		    m_textureManager->ResolveTextureReferenceOrDefault(desc.FindTextureReference(TextureGroup::SubsurfaceColor), DefaultTexture::Black),
		    m_textureManager->ResolveTextureReferenceOrDefault(desc.FindTextureReference(TextureGroup::SubsurfaceStrength), DefaultTexture::Black)};

		auto textureBindingSet = m_renderHardwareInterface->CreateBindingSet(
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
			return;
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
			}

			textures[slot]->WriteShaderResourceView(textureBindingSet->GetCpuDescriptorHandle(slot));
		}

		material.textureBindingSet = textureBindingSet.get();
		m_materialTextureBindingSets.push_back(std::move(textureBindingSet));
		m_cachedMaterialData.push_back(material);
	};

	if (materialSnapshot.HasMaterials())
	{
		m_cachedMaterialSnapshot = materialSnapshot;
		m_cachedMaterialData.reserve(materialSnapshot.materialDescs.size());
		m_materialTextureBindingSets.reserve(materialSnapshot.materialDescs.size());

		for (const auto& desc : materialSnapshot.materialDescs)
		{
			buildMaterialTable(desc);
		}
	}
	else
	{
		m_cachedMaterialData.reserve(1);
		m_materialTextureBindingSets.reserve(1);

		MaterialDesc defaultMaterial;
		defaultMaterial.name = "Renderer_DefaultMaterial";
		buildMaterialTable(defaultMaterial);
	}

	m_materialCacheBuilt = true;
}

void MaterialCacheManager::Reset() noexcept
{
	ReleaseMaterialTextureBindingSets();
	m_cachedMaterialData.clear();
	m_cachedMaterialSnapshot.Reset();
	m_materialCacheBuilt = false;
	m_cachedFromSceneMaterials = false;
}

void MaterialCacheManager::ReleaseMaterialTextureBindingSets() noexcept
{
	m_materialTextureBindingSets.clear();
}