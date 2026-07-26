#pragma once

#include "Rendering/RenderFrameDynamicData.h"

#include <memory>

class GPUMeshCache;
class MaterialCacheManager;
class TextureManager;
class RenderWorld;
class RenderMeshDrawBuilder;
struct RenderSceneData;

class RenderSceneDataBuilder final
{
  public:
	RenderSceneDataBuilder(MaterialCacheManager& materialCache, GPUMeshCache& gpuMeshCache, TextureManager& textureManager) noexcept;
	~RenderSceneDataBuilder() noexcept;

	RenderSceneDataBuilder(const RenderSceneDataBuilder&) = delete;
	RenderSceneDataBuilder& operator=(const RenderSceneDataBuilder&) = delete;
	RenderSceneDataBuilder(RenderSceneDataBuilder&&) = delete;
	RenderSceneDataBuilder& operator=(RenderSceneDataBuilder&&) = delete;

	RenderSceneData Build(const RenderWorld& world, const RenderFrameDynamicData& dynamic);

  private:
	void BuildMaterials(const RenderWorld& world, RenderSceneData& sceneData) const;
	void BuildSky(const RenderWorld& world, RenderSceneData& sceneData) const;

	MaterialCacheManager& m_materialCache;
	TextureManager& m_textureManager;
	std::unique_ptr<RenderMeshDrawBuilder> m_meshDrawBuilder;
};
