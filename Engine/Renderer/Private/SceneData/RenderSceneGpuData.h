#pragma once

#include "Renderer/Public/FrameGraph/FrameGraphBufferHandle.h"
#include "RHI/Public/Resources/RhiResourceHandles.h"
#include "ShaderData/RenderViewLightingData.h"

#include <cstdint>

class FrameGraph;
class FrameGraphBuilder;

struct RenderSceneGpuBuffer final
{
	RhiOwnedResourceHandle Resource = {};
	std::uint64_t SizeInBytes = 0;
	std::uint32_t StrideInBytes = 0;

	bool IsValid() const noexcept;
	explicit operator bool() const noexcept;
};

struct RenderSceneGpuLightingData final
{
	ViewLightingData Constants = {};
	RenderSceneGpuBuffer DirectionalLights;
	RenderSceneGpuBuffer PointLights;
	RenderSceneGpuBuffer SpotLights;
	RenderSceneGpuBuffer RectLights;
};

struct RenderSceneGpuGeometryData final
{
	RenderSceneGpuBuffer MeshInstances;
	RenderSceneGpuBuffer MeshInstanceSlots;
	RenderSceneGpuBuffer JointMatrices;
	RenderSceneGpuBuffer PreviousJointMatrices;
	RenderSceneGpuBuffer MorphWeights;
	RenderSceneGpuBuffer PreviousMorphWeights;

	bool HasMeshInstances() const noexcept;
	bool HasSkinning() const noexcept;
	bool HasMorphing() const noexcept;
};

struct RenderSceneGpuRayTracingData final
{
	RenderSceneGpuBuffer Vertices;
	RenderSceneGpuBuffer SkinInfluences;
	RenderSceneGpuBuffer MorphTargetDeltas;
	RenderSceneGpuBuffer Indices;
	RenderSceneGpuBuffer Instances;
	RenderSceneGpuBuffer Materials;
	std::uint32_t InstanceCount = 0u;
	std::uint32_t MaterialCount = 0u;

	bool IsValid() const noexcept;
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
	FrameGraphBufferHandle MeshInstanceSlots = FrameGraphBufferHandle::Invalid();
	FrameGraphBufferHandle JointMatrices = FrameGraphBufferHandle::Invalid();
	FrameGraphBufferHandle PreviousJointMatrices = FrameGraphBufferHandle::Invalid();
	FrameGraphBufferHandle MorphWeights = FrameGraphBufferHandle::Invalid();
	FrameGraphBufferHandle PreviousMorphWeights = FrameGraphBufferHandle::Invalid();
};

struct RenderSceneGpuRayTracingResources final
{
	FrameGraphBufferHandle Vertices = FrameGraphBufferHandle::Invalid();
	FrameGraphBufferHandle SkinInfluences = FrameGraphBufferHandle::Invalid();
	FrameGraphBufferHandle MorphTargetDeltas = FrameGraphBufferHandle::Invalid();
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

RenderSceneGpuResources DeclareRenderSceneGpuResources(FrameGraphBuilder& builder);
void BindRenderSceneGpuResources(
    FrameGraph& graph,
    const RenderSceneGpuResources& resources,
    const RenderSceneGpuData& sceneGpuData) noexcept;
