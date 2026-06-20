#pragma once

#include "Frame/RayTracingSceneFrameGraphResources.h"
#include "Frame/Targets/FrameRenderTargets.h"
#include "Renderer/Public/Denoising/ShadowDenoiseContract.h"
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
};

struct FrameAssemblyPersistentResources final
{
	FrameGraphAccelerationStructureHandle SceneTlas = FrameGraphAccelerationStructureHandle::Invalid();
	RayTracingSceneFrameGraphResources RayTracing = {};
};

struct FrameAssemblyHistoryResources final
{
	ShadowDenoiseContract::ShadowDenoiseTextures ShadowDenoiser = {};

	bool HasShadowHistory() const noexcept { return ShadowDenoiser.DenoiseHistory.IsValid(); }
};

struct FrameAssemblyProviderResources final
{
	FrameGraphTextureHandle HudlessSceneColor = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle Depth = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle MotionVectors = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle FinalOutputColor = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle Exposure = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle Normals = FrameGraphTextureHandle::Invalid();
};

struct FrameAssemblyViewportProducts final
{
	FrameGraphTextureHandle SceneColor = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle FinalSceneColor = FrameGraphTextureHandle::Invalid();
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
	FrameAssemblyProviderResources ProviderInputs = {};
	FrameAssemblyViewportProducts ViewportProducts = {};
};
