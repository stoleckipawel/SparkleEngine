#pragma once

#include <cstdint>

class GpuMeshCache;
class RenderDeformationPreparation;
class RenderScene;
class TextureCache;
struct RenderPrimitive;
struct RenderScenePreparationRun;
struct PreparedRenderScene;
struct ResolvedRenderPrimitive;

class RenderScenePreparationInputResolver final
{
public:
	RenderScenePreparationInputResolver(GpuMeshCache& gpuMeshCache, TextureCache& textureCache) noexcept;

	void Resolve(RenderScene& scene, RenderDeformationPreparation& deformationPreparation, RenderScenePreparationRun& run);

private:
	void ResolvePrimitives(const RenderScene& scene, RenderScenePreparationRun& run);
	ResolvedRenderPrimitive ResolvePrimitive(
	    const RenderScene& scene,
	    const RenderPrimitive& primitive,
	    std::uint32_t materialGeneration,
	    PreparedRenderScene& preparedScene);
	void ResolveInstanceGroups(const RenderScene& scene, RenderScenePreparationRun& run) const;
	void ResolveSky(const RenderScene& scene, PreparedRenderScene& preparedScene) const;

	GpuMeshCache* m_gpuMeshCache = nullptr;
	TextureCache* m_textureCache = nullptr;
};
