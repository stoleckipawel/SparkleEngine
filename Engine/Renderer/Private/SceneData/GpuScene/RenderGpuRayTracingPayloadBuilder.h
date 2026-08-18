#pragma once

#include "SceneData/GpuScene/RenderGpuScenePayloads.h"

class GpuMeshCache;
struct MaterialData;
struct MeshDraw;
struct RenderRayTracingBlasInput;
struct RenderSceneData;

class RenderGpuRayTracingPayloadBuilder final
{
public:
	static void Build(const RenderSceneData& sceneData, const GpuMeshCache& meshes, RenderGpuRayTracingPayloads& payloads);

private:
	struct BuildState;

	static void AppendMaterials(const RenderSceneData& sceneData, RenderGpuRayTracingPayloads& payloads);
	static void PrepareInstances(const RenderSceneData& sceneData, RenderGpuRayTracingPayloads& payloads, BuildState& state);
	static void AppendInstance(
	    const RenderRayTracingBlasInput& input,
	    const RenderSceneData& sceneData,
	    const GpuMeshCache& meshes,
	    RenderGpuRayTracingPayloads& payloads,
	    BuildState& state);
	static std::uint32_t BuildMaterialFlags(const MaterialData& material) noexcept;
	static std::uint32_t BuildGeometryFlags(const MeshDraw& draw, const MaterialData& material) noexcept;
	static void Clear(RenderGpuRayTracingPayloads& payloads) noexcept;
};
