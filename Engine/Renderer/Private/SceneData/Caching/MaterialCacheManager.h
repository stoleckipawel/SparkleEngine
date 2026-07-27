#pragma once

#include "SceneData/MaterialTextureTable.h"
#include "Rendering/RenderInputFrame.h"
#include "SceneData/MaterialData.h"

#include <cstdint>
#include <memory>
#include <vector>

struct RenderSceneData;
class RenderBindingSet;
class RenderHardwareInterface;
class TextureManager;
struct MaterialDesc;

class MaterialCacheManager final
{
  public:
	MaterialCacheManager(TextureManager& textureManager, RenderHardwareInterface& renderHardwareInterface) noexcept;
	~MaterialCacheManager() noexcept;

	MaterialCacheManager(const MaterialCacheManager&) = delete;
	MaterialCacheManager& operator=(const MaterialCacheManager&) = delete;
	MaterialCacheManager(MaterialCacheManager&&) = delete;
	MaterialCacheManager& operator=(MaterialCacheManager&&) = delete;

	void BuildMaterials(
	    const RenderMaterialTable& materials,
	    std::uint64_t sourceRevision,
	    RenderSceneData& sceneData);
	void Reset() noexcept;

  private:
	struct Build;

	bool Rebuild(
	    const RenderMaterialTable& materials,
	    std::uint64_t sourceRevision,
	    std::uint64_t textureRevision);
	bool BuildMaterial(
	    const MaterialDesc& desc,
	    std::uint32_t materialIndex,
	    std::uint64_t generation,
	    Build& build);
	void PublishMaterialTextureTable(RenderSceneData& sceneData) const noexcept;
	std::uint64_t GetNextGeneration() const noexcept;

	TextureManager& m_textureManager;
	RenderHardwareInterface& m_renderHardwareInterface;
	std::vector<MaterialData> m_cachedMaterialData;
	std::vector<std::unique_ptr<RenderBindingSet>> m_materialTextureBindingSets;
	MaterialTextureTable m_materialTextureTable;
	std::uint64_t m_sourceRevision = 0u;
	std::uint64_t m_textureRevision = 0u;
	std::uint64_t m_generation = 0u;
	bool m_materialCacheBuilt = false;
};
