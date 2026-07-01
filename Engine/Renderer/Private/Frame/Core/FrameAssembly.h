#pragma once

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

struct FrameAssemblyUpscalerProviderResources final
{
	FrameGraphTextureHandle ScalingInputColor = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle ScalingOutputColor = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle Depth = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle MotionVectors = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle Exposure = FrameGraphTextureHandle::Invalid();
};

struct FrameAssemblyIndirectReconstructionProviderResources final
{
	FrameGraphTextureHandle NoisyIndirectDiffuse = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle NoisyIndirectSpecular = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle DemodulatedIndirectDiffuse = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle DemodulatedIndirectSpecular = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle DiffuseAlbedo = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle SpecularAlbedo = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle MaterialGuide = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle DiffuseSampleGuide = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle SpecularSampleGuide = FrameGraphTextureHandle::Invalid();
};

struct FrameAssemblyDenoiserProviderResources final
{
	FrameGraphTextureHandle Depth = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle MotionVectors = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle Exposure = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle Normals = FrameGraphTextureHandle::Invalid();
	FrameAssemblyIndirectReconstructionProviderResources IndirectReconstruction = {};
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
	FrameAssemblyUpscalerProviderResources UpscalerProviderInputs = {};
	FrameAssemblyDenoiserProviderResources DenoiserProviderInputs = {};
	FrameAssemblyViewportProducts ViewportProducts = {};
};
