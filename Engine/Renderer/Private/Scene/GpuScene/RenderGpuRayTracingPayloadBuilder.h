#pragma once

#include "Scene/GpuScene/RenderGpuScenePayloads.h"

#include <cstdint>

class GpuMeshCache;
struct MaterialData;
struct MeshDraw;
struct RenderRayTracingBlasInput;
struct PreparedRenderScene;

class RenderGpuRayTracingPayloadBuilder final
{
public:
	static void Build(const PreparedRenderScene& preparedScene, const GpuMeshCache& meshes, RenderGpuRayTracingPayloads& payloads);

private:
	struct BuildState;

	static void AppendMaterials(const PreparedRenderScene& preparedScene, RenderGpuRayTracingPayloads& payloads);
	static void PrepareInstances(const PreparedRenderScene& preparedScene, RenderGpuRayTracingPayloads& payloads, BuildState& state);
	static void AppendInstance(
	    const RenderRayTracingBlasInput& input,
	    const PreparedRenderScene& preparedScene,
	    const GpuMeshCache& meshes,
	    RenderGpuRayTracingPayloads& payloads,
	    BuildState& state);
	static std::uint32_t BuildMaterialFlags(const MaterialData& material) noexcept;
	static std::uint32_t BuildGeometryFlags(const MeshDraw& draw, const MaterialData& material) noexcept;
	static void Clear(RenderGpuRayTracingPayloads& payloads) noexcept;
};
