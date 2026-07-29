#pragma once

#include "SceneData/GpuScene/RenderGpuScenePayloads.h"

class GpuMeshCache;
struct MaterialData;
struct MeshDraw;
struct RenderRayTracingBlasInput;
struct RenderSceneData;

class RenderGpuScenePayloadBuilder final
{
  public:
	static void BuildLighting(const RenderSceneData& sceneData, RenderGpuLightingPayloads& payloads);
	static void BuildRayTracing(const RenderSceneData& sceneData, const GpuMeshCache& meshes, RenderGpuRayTracingPayloads& payloads);

  private:
	struct RayTracingBuildState;

	static void AppendRayTracingMaterials(const RenderSceneData& sceneData, RenderGpuRayTracingPayloads& payloads);
	static void PrepareRayTracingInstances(
	    const RenderSceneData& sceneData,
	    RenderGpuRayTracingPayloads& payloads,
	    RayTracingBuildState& state);
	static void AppendRayTracingInstance(
	    const RenderRayTracingBlasInput& input,
	    const RenderSceneData& sceneData,
	    const GpuMeshCache& meshes,
	    RenderGpuRayTracingPayloads& payloads,
	    RayTracingBuildState& state);
	static std::uint32_t BuildMaterialFlags(const MaterialData& material) noexcept;
	static std::uint32_t BuildGeometryFlags(const MeshDraw& draw, const MaterialData& material) noexcept;
	static void ClearRayTracingPayloads(RenderGpuRayTracingPayloads& payloads) noexcept;
};
