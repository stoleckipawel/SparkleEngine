#pragma once

#include "FrameGraph/FrameGraphBufferHandle.h"

class FrameGraph;
class FrameGraphBuilder;
struct RenderSceneGpuBindings;

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
    const RenderSceneGpuBindings& sceneGpuBindings) noexcept;
