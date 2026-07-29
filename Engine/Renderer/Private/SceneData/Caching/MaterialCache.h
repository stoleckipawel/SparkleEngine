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
class TextureCache;
struct MaterialDesc;

class MaterialCache final
{
  public:
	MaterialCache(TextureCache& textureCache, RenderHardwareInterface& renderHardwareInterface) noexcept;
	~MaterialCache() noexcept;

	MaterialCache(const MaterialCache&) = delete;
	MaterialCache& operator=(const MaterialCache&) = delete;
	MaterialCache(MaterialCache&&) = delete;
	MaterialCache& operator=(MaterialCache&&) = delete;

	void BuildMaterials(
	    const RenderMaterialTable& materials,
	    std::uint64_t sourceRevision,
	    RenderSceneData& sceneData);
	void Reset() noexcept;

  private:
	struct RebuildOutput;

	void Rebuild(
	    const RenderMaterialTable& materials,
	    std::uint64_t sourceRevision,
	    std::uint64_t textureRevision);
	void BuildMaterial(
	    const MaterialDesc& desc,
	    std::uint32_t materialIndex,
	    std::uint64_t generation,
	    RebuildOutput& output);
	void PublishMaterialTextureTable(RenderSceneData& sceneData) const noexcept;
	std::uint64_t GetNextGeneration() const noexcept;

	TextureCache& m_textureCache;
	RenderHardwareInterface& m_renderHardwareInterface;
	std::vector<MaterialData> m_cachedMaterialData;
	std::vector<std::unique_ptr<RenderBindingSet>> m_materialTextureBindingSets;
	MaterialTextureTable m_materialTextureTable;
	std::uint64_t m_sourceRevision = 0u;
	std::uint64_t m_textureRevision = 0u;
	std::uint64_t m_generation = 0u;
	bool m_materialCacheBuilt = false;
};
