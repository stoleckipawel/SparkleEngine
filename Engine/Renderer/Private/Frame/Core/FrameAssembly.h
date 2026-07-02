#pragma once

#include "Frame/Core/FrameProviderResources.h"
#include "Frame/Reference/ReferenceRenderTargets.h"
#include "Frame/RayTracing/RayTracingSceneFrameGraphResources.h"
#include "Frame/Targets/FrameRenderTargets.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureHandle.h"

struct FrameAssemblyImportedResources final
{
	FrameGraphTextureHandle BackBuffer = FrameGraphTextureHandle::Invalid();
};

struct FrameAssemblyTransientResources final
{
	SceneRenderTargets Scene = {};
	GBufferRenderTargets GBuffer = {};
	LightingRenderTargets Lighting = {};
	ReferenceRenderTargets Reference = {};
	FrameGraphTextureHandle ShadowVisibilitySignal = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle ShadowLightSample = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle Exposure = FrameGraphTextureHandle::Invalid();
};

struct FrameAssemblyPersistentResources final
{
	FrameGraphAccelerationStructureHandle SceneTlas = FrameGraphAccelerationStructureHandle::Invalid();
	RayTracingSceneFrameGraphResources RayTracing = {};
};

struct FrameAssemblyHistoryResources final
{
	FrameGraphTextureHandle PreviousExposure = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle CurrentExposure = FrameGraphTextureHandle::Invalid();

	bool HasExposureHistory() const noexcept { return PreviousExposure.IsValid() && CurrentExposure.IsValid(); }
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

struct FrameAssemblyResourceLayout final
{
	FrameAssemblyImportedResources Imported = {};
	FrameAssemblyTransientResources Transient = {};
	FrameAssemblyPersistentResources Persistent = {};
	FrameAssemblyHistoryResources History = {};
	FrameUpscalerProviderResources UpscalerProviderInputs = {};
	FrameRayReconstructionProviderResources RayReconstructionProviderInputs = {};
	FrameAssemblyViewportProducts ViewportProducts = {};
	bool FinalSceneColorProduced = false;
};
