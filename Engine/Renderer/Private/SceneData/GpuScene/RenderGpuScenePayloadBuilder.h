#pragma once

#include "SceneData/GpuScene/RenderGpuScenePayloads.h"

class GPUMeshCache;
struct MaterialData;
struct MeshDraw;
struct RenderSceneData;
struct VertexSkinInfluence;

class RenderGpuScenePayloadBuilder final
{
  public:
	static void BuildLighting(
	    const RenderSceneData& sceneData,
	    RenderGpuLightingPayloads& payloads);
	static void BuildRayTracing(
	    const RenderSceneData& sceneData,
	    const GPUMeshCache& meshes,
	    RenderGpuRayTracingPayloads& payloads);

  private:
	static VertexSkinInfluenceData ConvertSkinInfluence(
	    const VertexSkinInfluence& influence) noexcept;
	static std::uint32_t BuildMaterialFlags(
	    const MaterialData& material) noexcept;
	static std::uint32_t BuildGeometryFlags(
	    const MeshDraw& draw,
	    const MaterialData* material) noexcept;
	static RayTracingHitInstance BuildRejectedInstance(
	    const MeshDraw& draw,
	    const MaterialData* material,
	    std::uint32_t rejectionReason) noexcept;
	static void ClearRayTracingPayloads(
	    RenderGpuRayTracingPayloads& payloads) noexcept;
};
