#pragma once

#include "RHI/Public/D3D12/Resources/D3D12ConstantBufferData.h"

#include <cstdint>

class GameScene;
class MaterialCacheManager;
class RenderCamera;
struct RenderSceneView;

struct RenderSceneViewportDesc
{
	const RenderCamera* camera = nullptr;
	std::uint32_t width = 0;
	std::uint32_t height = 0;
};

class RenderSceneViewBuilder final
{
  public:
	explicit RenderSceneViewBuilder(MaterialCacheManager& materialCache) noexcept;
	~RenderSceneViewBuilder() noexcept = default;

	RenderSceneViewBuilder(const RenderSceneViewBuilder&) = delete;
	RenderSceneViewBuilder& operator=(const RenderSceneViewBuilder&) = delete;
	RenderSceneViewBuilder(RenderSceneViewBuilder&&) = delete;
	RenderSceneViewBuilder& operator=(RenderSceneViewBuilder&&) = delete;

	RenderSceneView BuildViewport(const GameScene& gameScene, const RenderSceneViewportDesc& viewportDesc);

	void PopulatePerViewLightingData(const RenderSceneView& renderSceneView, PerViewConstantBufferData& perViewData) const noexcept;

  private:
	void BuildMeshDraws(const GameScene& gameScene, RenderSceneView& renderSceneView) const;
	void BuildLighting(const GameScene& gameScene, RenderSceneView& renderSceneView) const noexcept;

	MaterialCacheManager* m_materialCache = nullptr;
};