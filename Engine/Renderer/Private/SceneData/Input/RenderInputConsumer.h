#pragma once

#include "Rendering/RenderInputFrame.h"
#include "SceneData/Input/Validation/RenderInputFrameValidator.h"

#include <optional>
#include <string>
#include <utility>

class GPUMeshCache;
class MaterialCacheManager;
class RenderDeviceServices;
class RenderRayTracingScene;
class RenderWorld;
class TextureManager;

struct RenderInputConsumeResult final
{
	bool Accepted = false;
	bool SceneReset = false;
	std::string Diagnostic;
};

class RenderInputConsumer final
{
  public:
	RenderInputConsumer(
	    RenderWorld& world,
	    RenderDeviceServices& backend,
	    GPUMeshCache& meshCache,
	    TextureManager& textureManager,
	    MaterialCacheManager& materialCache,
	    RenderRayTracingScene* rayTracingScene) noexcept;

	bool Submit(RenderInputFrame input);
	RenderInputConsumeResult ConsumePending() noexcept;
	const RenderFrameDynamicData& GetDynamicData() const noexcept { return m_dynamic; }

  private:
	void ResetSceneResources() noexcept;
	RenderWorld* m_world = nullptr;
	RenderDeviceServices* m_backend = nullptr;
	GPUMeshCache* m_meshCache = nullptr;
	TextureManager* m_textureManager = nullptr;
	MaterialCacheManager* m_materialCache = nullptr;
	RenderRayTracingScene* m_rayTracingScene = nullptr;
	std::optional<RenderInputFrame> m_pending;
	RenderInputFrameValidator m_validator;
	RenderFrameDynamicData m_dynamic;
};
