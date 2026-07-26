#pragma once

#include "Renderer/Public/FrameGraph/FrameGraphBufferHandle.h"
#include "Resources/OwnedStructuredBuffer.h"
#include "ShaderData/RenderViewLightingData.h"

#include <cstdint>
#include <memory>

class FrameGraph;
class FrameGraphBuilder;
class RhiResourceService;
struct RenderSceneData;

struct RenderSceneGpuLightingData final
{
	ViewLightingData Constants = {};
	OwnedStructuredBuffer DirectionalLights;
	OwnedStructuredBuffer PointLights;
	OwnedStructuredBuffer SpotLights;
	OwnedStructuredBuffer RectLights;
};

struct RenderSceneGpuGeometryData final
{
	OwnedStructuredBuffer MeshInstances;
	OwnedStructuredBuffer JointMatrices;
	OwnedStructuredBuffer PreviousJointMatrices;

	bool HasMeshInstances() const noexcept { return MeshInstances.IsValid(); }
	bool HasSkinning() const noexcept { return JointMatrices.IsValid() && PreviousJointMatrices.IsValid(); }
};

struct RenderSceneGpuRayTracingData final
{
	OwnedStructuredBuffer Vertices;
	OwnedStructuredBuffer SkinInfluences;
	OwnedStructuredBuffer Indices;
	OwnedStructuredBuffer Instances;
	OwnedStructuredBuffer Materials;
	std::uint32_t InstanceCount = 0u;
	std::uint32_t MaterialCount = 0u;

	bool IsValid() const noexcept
	{
		return Vertices && SkinInfluences && Indices && Instances && Materials && InstanceCount > 0u && MaterialCount > 0u;
	}
};

struct RenderSceneGpuData final
{
	RenderSceneGpuLightingData Lighting = {};
	RenderSceneGpuGeometryData Geometry = {};
	RenderSceneGpuRayTracingData RayTracing = {};
};

struct RenderSceneGpuLightingResources final
{
	FrameGraphBufferHandle DirectionalLights = FrameGraphBufferHandle::Invalid();
	FrameGraphBufferHandle PointLights = FrameGraphBufferHandle::Invalid();
	FrameGraphBufferHandle SpotLights = FrameGraphBufferHandle::Invalid();
	FrameGraphBufferHandle RectLights = FrameGraphBufferHandle::Invalid();
};

struct RenderSceneGpuGeometryResources final
{
	FrameGraphBufferHandle MeshInstances = FrameGraphBufferHandle::Invalid();
	FrameGraphBufferHandle JointMatrices = FrameGraphBufferHandle::Invalid();
	FrameGraphBufferHandle PreviousJointMatrices = FrameGraphBufferHandle::Invalid();
};

struct RenderSceneGpuRayTracingResources final
{
	FrameGraphBufferHandle Vertices = FrameGraphBufferHandle::Invalid();
	FrameGraphBufferHandle SkinInfluences = FrameGraphBufferHandle::Invalid();
	FrameGraphBufferHandle Indices = FrameGraphBufferHandle::Invalid();
	FrameGraphBufferHandle Instances = FrameGraphBufferHandle::Invalid();
	FrameGraphBufferHandle Materials = FrameGraphBufferHandle::Invalid();
};

struct RenderSceneGpuResources final
{
	RenderSceneGpuLightingResources Lighting = {};
	RenderSceneGpuGeometryResources Geometry = {};
	RenderSceneGpuRayTracingResources RayTracing = {};
};

class PersistentRenderGpuScene final
{
  public:
	explicit PersistentRenderGpuScene(
	    RhiResourceService& resourceService);
	~PersistentRenderGpuScene() noexcept;

	PersistentRenderGpuScene(const PersistentRenderGpuScene&) = delete;
	PersistentRenderGpuScene& operator=(const PersistentRenderGpuScene&) = delete;

	const RenderSceneGpuData& Update(const RenderSceneData& sceneData);
	void Reset() noexcept;

  private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

RenderSceneGpuResources DeclareRenderSceneGpuResources(FrameGraphBuilder& builder);
void BindRenderSceneGpuResources(
    FrameGraph& graph,
    const RenderSceneGpuResources& resources,
    const RenderSceneGpuData& sceneGpuData) noexcept;
