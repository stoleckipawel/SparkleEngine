#pragma once

#include <cstdint>

class MaterialCacheManager;
class RenderCamera;
struct RenderSceneSnapshot;
struct SceneView;

class SceneViewBuilder final
{
  public:
	explicit SceneViewBuilder(MaterialCacheManager& materialCache) noexcept;
	~SceneViewBuilder() noexcept = default;

	SceneViewBuilder(const SceneViewBuilder&) = delete;
	SceneViewBuilder& operator=(const SceneViewBuilder&) = delete;
	SceneViewBuilder(SceneViewBuilder&&) = delete;
	SceneViewBuilder& operator=(SceneViewBuilder&&) = delete;

	SceneView Build(const RenderSceneSnapshot& sceneSnapshot, const RenderCamera& renderCamera, std::uint32_t width, std::uint32_t height);

  private:
	void BuildMeshDraws(const RenderSceneSnapshot& sceneSnapshot, SceneView& view) const;

	MaterialCacheManager* m_materialCache = nullptr;
};