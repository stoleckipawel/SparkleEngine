#pragma once

#include "Frame/Graph/RenderFrameGraphTargets.h"
#include "Resources/History/FrameHistory.h"
#include "Scene/GpuScene/RenderSceneFrameGraphResources.h"
#include "FrameGraph/FrameGraphAccelerationStructureHandle.h"
#include "FrameGraph/FrameGraphTextureHandle.h"

class FrameGraphBuilder;
struct RenderFrameGraphSettings;

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
	FrameGraphTextureHandle FinalColorLdr = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle SceneDepth = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle Normals = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle Radiance = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle RadianceSecondMoment = FrameGraphTextureHandle::Invalid();
	RenderProductSamplePrefix RadianceSamplePrefix = {};
	ViewportRenderProgress Progress = {};
};

struct RenderFrameGraphImportedSceneResources final
{
	FrameGraphTextureHandle Sky = FrameGraphTextureHandle::Invalid();
	RenderSceneGpuResources Scene = {};
};

struct RenderFrameGraphPresentationResources final
{
	FrameGraphTextureHandle SceneColorInput = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle ResolvedSceneColor = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle BackBuffer = FrameGraphTextureHandle::Invalid();
};

struct RenderFrameGraphResources final
{
	RenderFrameGraphTransientResources Transient = {};
	RenderFrameGraphImportedSceneResources ImportedScene = {};
	RenderFrameGraphPresentationResources Presentation = {};
	FrameGraphAccelerationStructureHandle SceneTlas = FrameGraphAccelerationStructureHandle::Invalid();
	FrameHistoryResourceLayout History = {};
	ViewportFrameProducts ViewportProducts = {};
};

RenderFrameGraphResources CreateRenderFrameGraphResources(FrameGraphBuilder& builder, const RenderFrameGraphSettings& settings);
FrameGraphTextureHandle CreateResolvedSceneColorTarget(FrameGraphBuilder& builder, RenderViewportExtent outputExtent);
