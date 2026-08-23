#pragma once

#include "Frame/Graph/RenderFrameGraphTargets.h"
#include "Resources/History/FrameHistory.h"
#include "Renderer/Public/FrameGraph/FrameGraphAccelerationStructureHandle.h"
#include "Renderer/Public/FrameGraph/FrameGraphBufferHandle.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureHandle.h"

struct RenderFrameGraphTransientResources final
{
	SceneRenderTargets Scene = {};
	GBufferRenderTargets GBuffer = {};
	LightingRenderTargets Lighting = {};
	FrameGraphTextureHandle ShadowVisibilitySignal = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle DirectLightTemporalReservoirSample = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle DirectLightTemporalReservoirWeight = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle Exposure = FrameGraphTextureHandle::Invalid();
};

struct ViewportFrameProducts final
{
	FrameGraphTextureHandle FinalSceneColor = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle SceneDepth = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle Normals = FrameGraphTextureHandle::Invalid();
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

struct RenderFrameGraphImportedSceneResources final
{
	FrameGraphTextureHandle Sky = FrameGraphTextureHandle::Invalid();
	RenderSceneGpuResources Scene = {};
};

struct RenderFrameGraphResources final
{
	RenderFrameGraphTransientResources Transient = {};
	RenderFrameGraphImportedSceneResources ImportedScene = {};
	FrameGraphAccelerationStructureHandle SceneTlas = FrameGraphAccelerationStructureHandle::Invalid();
	FrameHistoryResourceLayout History = {};
	ViewportFrameProducts ViewportProducts = {};
	FrameGraphTextureHandle ResolvedSceneColor = FrameGraphTextureHandle::Invalid();
};
