#include "PCH.h"
#include "Frame/Deferred/GBuffer.h"

#include "Frame/Deferred/GBufferFormats.h"
#include "Frame/Deferred/SceneDepth.h"
#include "Frame/GBuffer/RaytracedGBuffer.h"
#include "Frame/GBuffer/RasterizedGBuffer.h"
#include "Frame/GBuffer/SkyMotionVectors.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Renderer/Public/Debug/RendererCVars.h"
#include "FrameGraph/FrameGraphTextureDesc.h"

#include <array>

class GBufferOperations final
{
  public:
	static FrameGraphTextureHandle CreateGBufferColor(
	    FrameGraphBuilder& builder,
	    const char* name,
	    RenderViewportExtent sceneExtent,
	    PixelFormat format,
	    std::array<float, 4> clearColor)
	{
		FrameGraphTextureDesc desc = FrameGraphTextureDesc::CreateColor(name, sceneExtent.Width, sceneExtent.Height, format);
		desc.clearColor = clearColor;
		return builder.CreateTexture(desc);
	}

	static FrameGraphTextureHandle CreateGBufferDeviceZ(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent, GBufferMode gBufferMode)
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
};

GBufferRenderTargets CreateGBufferRenderTargets(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent, GBufferMode gBufferMode)
{
	GBufferRenderTargets targets{};
	targets.BaseColor = GBufferOperations::CreateGBufferColor(builder, "GBufferBaseColor", sceneExtent, GBufferFormats::BaseColor, {0.0f, 0.0f, 0.0f, 1.0f});
	targets.Normal = GBufferOperations::CreateGBufferColor(builder, "GBufferNormal", sceneExtent, GBufferFormats::Normal, {0.0f, 0.0f, 1.0f, 0.0f});
	targets.Material = GBufferOperations::CreateGBufferColor(builder, "GBufferMaterial", sceneExtent, GBufferFormats::Material, {0.0f, 1.0f, 1.0f, 0.04f});
	targets.Emissive = GBufferOperations::CreateGBufferColor(builder, "GBufferEmissive", sceneExtent, GBufferFormats::Emissive, {0.0f, 0.0f, 0.0f, 0.0f});
	targets.Subsurface =
	    GBufferOperations::CreateGBufferColor(builder, "GBufferSubsurface", sceneExtent, GBufferFormats::Subsurface, {0.0f, 0.0f, 0.0f, 0.0f});
	targets.MotionVector =
	    GBufferOperations::CreateGBufferColor(builder, "GBufferMotionVector", sceneExtent, GBufferFormats::MotionVector, {0.0f, 0.0f, 0.0f, 0.0f});
	targets.DeviceZ = GBufferOperations::CreateGBufferDeviceZ(builder, sceneExtent, gBufferMode);
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
			AddRasterizedGBufferPass(builder, resources.Transient.GBuffer, resources.External);
			break;
		case GBufferMode::Raytraced:
			AddRaytracedGBufferPass(builder, resources.Transient.GBuffer, resources.SceneTlas, resources.External);
			break;
	}

	AddSkyMotionVectorPass(builder, resources.Transient.GBuffer);
	AddLinearizeDeviceZPass(builder, resources.Transient.GBuffer.DeviceZ, resources.Transient.Scene.SceneDepth);
}
