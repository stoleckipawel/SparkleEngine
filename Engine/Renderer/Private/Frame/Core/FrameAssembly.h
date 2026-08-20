#pragma once

#include "Frame/Targets/FrameRenderTargets.h"
#include "Resources/History/FrameHistory.h"
#include "Renderer/Public/FrameGraph/FrameGraphAccelerationStructureHandle.h"
#include "Renderer/Public/FrameGraph/FrameGraphBufferHandle.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureHandle.h"

struct FrameAssemblyTransientResources final
{
	SceneRenderTargets Scene = {};
	GBufferRenderTargets GBuffer = {};
	LightingRenderTargets Lighting = {};
	FrameGraphTextureHandle ShadowVisibilitySignal = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle DirectLightTemporalReservoirSample = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle DirectLightTemporalReservoirWeight = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle Exposure = FrameGraphTextureHandle::Invalid();
};

struct FrameAssemblyViewportProducts final
{
	FrameGraphTextureHandle SceneColor = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle FinalSceneColor = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle Exposure = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle SceneDepth = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle Normals = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle MotionVectors = FrameGraphTextureHandle::Invalid();
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

struct FrameAssemblyExternalResources final
{
	FrameGraphTextureHandle Sky = FrameGraphTextureHandle::Invalid();
	RenderSceneGpuResources Scene = {};
};

struct FrameAssemblyResourceLayout final
{
	FrameAssemblyTransientResources Transient = {};
	FrameAssemblyExternalResources External = {};
	FrameGraphAccelerationStructureHandle SceneTlas = FrameGraphAccelerationStructureHandle::Invalid();
	FrameHistoryResourceLayout History = {};
	FrameAssemblyViewportProducts ViewportProducts = {};
	bool FinalSceneColorProduced = false;
};
