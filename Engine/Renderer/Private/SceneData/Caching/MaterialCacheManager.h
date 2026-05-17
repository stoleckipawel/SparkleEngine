#pragma once

#include "RHI/Public/Descriptors/RhiDescriptorHandles.h"
#include "Scene/Materials/MaterialSnapshot.h"
#include "SceneData/MaterialData.h"

#include <cstdint>
#include <vector>

struct RenderSceneData;
class RenderHardwareInterface;
class TextureManager;

class MaterialCacheManager final
{
  public:
	MaterialCacheManager(TextureManager& textureManager, RenderHardwareInterface& renderHardwareInterface) noexcept;
	~MaterialCacheManager() noexcept;

	MaterialCacheManager(const MaterialCacheManager&) = delete;
	MaterialCacheManager& operator=(const MaterialCacheManager&) = delete;
	MaterialCacheManager(MaterialCacheManager&&) = delete;
	MaterialCacheManager& operator=(MaterialCacheManager&&) = delete;

	void BuildMaterials(const MaterialSnapshot& materialSnapshot, RenderSceneData& sceneData);
	void Rebuild(const MaterialSnapshot& materialSnapshot);
	void Reset() noexcept;

  private:
	void ReleaseMaterialTextureTables() noexcept;

	TextureManager* m_textureManager = nullptr;
	RenderHardwareInterface* m_renderHardwareInterface = nullptr;
	MaterialSnapshot m_cachedMaterialSnapshot;
	std::vector<MaterialData> m_cachedMaterialData;
	std::vector<RhiDescriptorTableHandle> m_materialTextureTables;
	bool m_materialCacheBuilt = false;
	bool m_cachedFromSceneMaterials = false;
};
