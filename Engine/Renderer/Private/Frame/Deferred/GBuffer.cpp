#include "PCH.h"
#include "Frame/Deferred/GBuffer.h"

#include "Frame/Deferred/GBufferFormats.h"
#include "Frame/Deferred/SceneDepth.h"
#include "Frame/GBuffer/RaytracedGBuffer.h"
#include "Frame/GBuffer/RasterizedGBuffer.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Renderer/Public/Debug/RendererCVars.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureDesc.h"

namespace
{
	FrameGraphTextureHandle CreateGBufferDeviceZ(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent, GBufferMode gBufferMode)
	{
		if (gBufferMode == GBufferMode::Raytraced)
		{
			return builder.CreateTexture(
			    FrameGraphTextureDesc::CreateColor(
			        "GBufferDeviceZ",
			        sceneExtent.Width,
			        sceneExtent.Height,
			        GBufferFormats::RaytracedDeviceZ));
		}

		return builder.CreateTexture(
		    FrameGraphTextureDesc::CreateDepthTarget(
		        "GBufferDeviceZ",
		        sceneExtent.Width,
		        sceneExtent.Height,
		        GBufferFormats::RasterizedDeviceZ));
	}
}

GBufferRenderTargets CreateGBufferRenderTargets(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent, GBufferMode gBufferMode)
{
	GBufferRenderTargets targets{};
	targets.BaseColor = builder.CreateTexture(
	    FrameGraphTextureDesc::CreateColor(
	        "GBufferBaseColor",
	        sceneExtent.Width,
	        sceneExtent.Height,
	        GBufferFormats::BaseColor));
	targets.Normal = builder.CreateTexture(
	    FrameGraphTextureDesc::CreateColor("GBufferNormal", sceneExtent.Width, sceneExtent.Height, GBufferFormats::Normal));
	targets.Material = builder.CreateTexture(
	    FrameGraphTextureDesc::CreateColor(
	        "GBufferMaterial",
	        sceneExtent.Width,
	        sceneExtent.Height,
	        GBufferFormats::Material));
	targets.Emissive = builder.CreateTexture(
	    FrameGraphTextureDesc::CreateColor(
	        "GBufferEmissive",
	        sceneExtent.Width,
	        sceneExtent.Height,
	        GBufferFormats::Emissive));
	targets.Subsurface = builder.CreateTexture(
	    FrameGraphTextureDesc::CreateColor(
	        "GBufferSubsurface",
	        sceneExtent.Width,
	        sceneExtent.Height,
	        GBufferFormats::Subsurface));
	targets.MotionVector = builder.CreateTexture(
	    FrameGraphTextureDesc::CreateColor(
	        "GBufferMotionVector",
	        sceneExtent.Width,
	        sceneExtent.Height,
	        GBufferFormats::MotionVector));
	targets.DeviceZ = CreateGBufferDeviceZ(builder, sceneExtent, gBufferMode);
	return targets;
}

void AddGBufferPasses(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent, FrameAssemblyResourceLayout& resources)
{
	const GBufferMode gBufferMode = CVarGBufferMode.Get();
	resources.Transient.GBuffer = CreateGBufferRenderTargets(builder, sceneExtent, gBufferMode);
	resources.ViewportProducts.Normals = resources.Transient.GBuffer.Normal;
	resources.ViewportProducts.MotionVectors = resources.Transient.GBuffer.MotionVector;

	switch (gBufferMode)
	{
		case GBufferMode::Rasterized:
		default:
			AddRasterizedGBufferPass(builder, resources.Transient.GBuffer);
			break;
		case GBufferMode::Raytraced:
			AddRaytracedGBufferPass(builder, resources.Transient.GBuffer, resources.SceneTlas);
			break;
	}

	AddLinearizeDeviceZPass(
	    builder,
	    resources.Transient.GBuffer.DeviceZ,
	    resources.Transient.Scene.SceneDepth);
}
