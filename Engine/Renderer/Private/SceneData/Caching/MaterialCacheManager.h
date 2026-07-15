#pragma once

#include "SceneData/MaterialTextureTable.h"
#include "Scene/Materials/MaterialSnapshot.h"
#include "SceneData/MaterialData.h"

#include <cstdint>
#include <memory>
#include <vector>

struct RenderSceneData;
class RenderBindingSet;
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
	void Reset() noexcept;

 private:
	bool Rebuild(const MaterialSnapshot& materialSnapshot);
	void PublishMaterialTextureTable(RenderSceneData& sceneData) const noexcept;
	std::uint64_t GetNextGeneration() const noexcept;

	TextureManager* m_textureManager = nullptr;
	RenderHardwareInterface* m_renderHardwareInterface = nullptr;
	MaterialSnapshot m_cachedMaterialSnapshot;
	std::vector<MaterialData> m_cachedMaterialData;
	std::vector<std::unique_ptr<RenderBindingSet>> m_materialTextureBindingSets;
	MaterialTextureTable m_materialTextureTable;
	std::uint64_t m_generation = 0u;
	bool m_materialCacheBuilt = false;
	bool m_cachedFromSceneMaterials = false;
};
