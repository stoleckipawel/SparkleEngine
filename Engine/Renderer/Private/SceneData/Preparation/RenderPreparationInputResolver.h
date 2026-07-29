#pragma once

#include "Rendering/RenderObjectId.h"

#include <DirectXMath.h>

#include <cstdint>
#include <span>

class GpuMeshCache;
class MaterialCache;
class RenderDeformationPreparation;
class RenderWorld;
class TextureCache;
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
	    MaterialCache& materialCache,
	    GpuMeshCache& gpuMeshCache,
	    TextureCache& textureCache) noexcept;

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
	ResolvedRenderObject ResolveObject(
	    const RenderProxy& proxy,
	    std::uint32_t materialGeneration,
	    std::span<const RenderPreviousWorldTransform> previousWorldTransforms,
	    RenderSceneData& sceneData);
	void ResolveInstanceGroups(const RenderWorld& world, RenderPreparationRun& run) const;
	void ResolveSky(const RenderWorld& world, RenderSceneData& sceneData) const;

	MaterialCache* m_materialCache = nullptr;
	GpuMeshCache* m_gpuMeshCache = nullptr;
	TextureCache* m_textureCache = nullptr;
};
