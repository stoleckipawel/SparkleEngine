#pragma once

#include "Frame/Core/FrameProviderResources.h"
#include "Frame/Targets/FrameRenderTargets.h"
#include "Renderer/Public/FrameGraph/FrameGraphAccelerationStructureHandle.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureHandle.h"

struct FrameAssemblyTransientResources final
{
	SceneRenderTargets Scene = {};
	GBufferRenderTargets GBuffer = {};
	LightingRenderTargets Lighting = {};
	FrameGraphTextureHandle ReferenceSceneColor = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle ShadowVisibilitySignal = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle DirectLightTemporalReservoirSample = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle DirectLightTemporalReservoirWeight = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle Exposure = FrameGraphTextureHandle::Invalid();
};

struct FrameAssemblyHistoryResources final
{
	FrameGraphTextureHandle PreviousExposure = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle CurrentExposure = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle PreviousDirectLightReservoirSample = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle PreviousDirectLightReservoirWeight = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle PreviousDirectLightReservoirSurface = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle CurrentDirectLightReservoirSample = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle CurrentDirectLightReservoirWeight = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle CurrentDirectLightReservoirSurface = FrameGraphTextureHandle::Invalid();

	bool HasExposureHistory() const noexcept { return PreviousExposure.IsValid() && CurrentExposure.IsValid(); }
	bool HasDirectLightReservoirHistory() const noexcept
	{
		return PreviousDirectLightReservoirSample.IsValid() &&
		       PreviousDirectLightReservoirWeight.IsValid() &&
		       PreviousDirectLightReservoirSurface.IsValid() &&
		       CurrentDirectLightReservoirSample.IsValid() &&
		       CurrentDirectLightReservoirWeight.IsValid() &&
		       CurrentDirectLightReservoirSurface.IsValid();
	}
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
	FrameAssemblyTransientResources Transient = {};
	FrameGraphAccelerationStructureHandle SceneTlas = FrameGraphAccelerationStructureHandle::Invalid();
	FrameAssemblyHistoryResources History = {};
	FrameUpscalerProviderResources UpscalerProviderInputs = {};
	FrameRayReconstructionProviderResources RayReconstructionProviderInputs = {};
	FrameAssemblyViewportProducts ViewportProducts = {};
	bool ReconstructedSceneColorProduced = false;
	bool FinalSceneColorProduced = false;
};
