#pragma once

#include "Rendering/RenderObjectId.h"

#include <DirectXMath.h>

#include <span>

class GPUMeshCache;
class MaterialCacheManager;
class RenderDeformationPreparation;
class RenderWorld;
class TextureManager;
struct Frustum;
struct RenderFrameDynamicData;
struct RenderProxy;
struct RenderPreparationRun;
struct RenderSceneData;
struct ResolvedRenderObject;

struct RenderPreviousWorldTransform final
{
	RenderObjectId Object;
	DirectX::XMFLOAT4X4 WorldMatrix = {};
};

class RenderPreparationInputResolver final
{
  public:
	RenderPreparationInputResolver(
	    MaterialCacheManager& materialCache,
	    GPUMeshCache& gpuMeshCache,
	    TextureManager& textureManager) noexcept;

	void Resolve(
	    const RenderWorld& world,
	    const RenderFrameDynamicData& dynamic,
	    const Frustum& frustum,
	    std::span<const RenderPreviousWorldTransform> previousWorldTransforms,
	    RenderDeformationPreparation& deformationPreparation,
	    RenderPreparationRun& run);

  private:
	void ResolveObjects(
	    const RenderWorld& world,
	    std::span<const RenderPreviousWorldTransform> previousWorldTransforms,
	    RenderPreparationRun& run);
	bool TryResolveObject(
	    const RenderProxy& proxy,
	    std::span<const RenderPreviousWorldTransform> previousWorldTransforms,
	    RenderSceneData& sceneData,
	    ResolvedRenderObject& output);
	void ResolveInstanceGroups(
	    const RenderWorld& world,
	    RenderPreparationRun& run) const;
	void ResolveSky(
	    const RenderWorld& world,
	    RenderSceneData& sceneData) const;

	MaterialCacheManager* m_materialCache = nullptr;
	GPUMeshCache* m_gpuMeshCache = nullptr;
	TextureManager* m_textureManager = nullptr;
};
