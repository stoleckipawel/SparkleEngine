#pragma once

#include "GameFramework/Public/Assets/MaterialDesc.h"
#include "D3D12DescriptorHandle.h"
#include "Renderer/Public/SceneData/MaterialData.h"

#include <vector>

class D3D12DescriptorHeapManager;
struct SceneView;
class TextureManager;
struct RenderSceneSnapshot;

class MaterialCacheManager final
{
  public:
	MaterialCacheManager(TextureManager& textureManager, D3D12DescriptorHeapManager& descriptorHeapManager) noexcept;
	~MaterialCacheManager() noexcept;

	MaterialCacheManager(const MaterialCacheManager&) = delete;
	MaterialCacheManager& operator=(const MaterialCacheManager&) = delete;
	MaterialCacheManager(MaterialCacheManager&&) = delete;
	MaterialCacheManager& operator=(MaterialCacheManager&&) = delete;

	void PopulateSceneMaterials(const RenderSceneSnapshot& sceneSnapshot, SceneView& view);
	void Rebuild(const RenderSceneSnapshot& sceneSnapshot);
	void Reset() noexcept;

  private:
	void ReleaseMaterialTextureTables() noexcept;

	TextureManager* m_textureManager = nullptr;
	D3D12DescriptorHeapManager* m_descriptorHeapManager = nullptr;
	std::vector<MaterialDesc> m_cachedMaterialDescs;
	std::vector<MaterialData> m_cachedMaterialData;
	std::vector<D3D12DescriptorHandle> m_materialTextureTables;
	bool m_materialCacheBuilt = false;
	bool m_materialCacheUsesLoadedMaterials = false;
};