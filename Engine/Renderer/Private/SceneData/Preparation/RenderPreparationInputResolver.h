#pragma once

#include <DirectXMath.h>

#include <cstdint>

class GpuMeshCache;
class RenderDeformationPreparation;
class RenderScene;
class TextureCache;
struct Frustum;
struct RenderPrimitive;
struct RenderPreparationRun;
struct RenderSceneData;
struct ResolvedRenderObject;

class RenderPreparationInputResolver final
{
public:
	RenderPreparationInputResolver(GpuMeshCache& gpuMeshCache, TextureCache& textureCache) noexcept;

	void Resolve(
	    RenderScene& scene,
	    const Frustum& frustum,
	    const DirectX::XMFLOAT3& cameraPosition,
	    RenderDeformationPreparation& deformationPreparation,
	    RenderPreparationRun& run);

private:
	void ResolveObjects(const RenderScene& scene, RenderPreparationRun& run);
	ResolvedRenderObject ResolveObject(
	    const RenderScene& scene,
	    const RenderPrimitive& primitive,
	    std::uint32_t materialGeneration,
	    RenderSceneData& sceneData);
	void ResolveInstanceGroups(const RenderScene& scene, RenderPreparationRun& run) const;
	void ResolveSky(const RenderScene& scene, RenderSceneData& sceneData) const;

	GpuMeshCache* m_gpuMeshCache = nullptr;
	TextureCache* m_textureCache = nullptr;
};
