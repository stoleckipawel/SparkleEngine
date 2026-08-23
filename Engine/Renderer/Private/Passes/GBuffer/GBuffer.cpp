#include "PCH.h"
#include "Passes/GBuffer/GBuffer.h"

#include "Passes/GBuffer/GBufferFormats.h"
#include "Passes/GBuffer/SceneDepth.h"
#include "Passes/GBuffer/RaytracedGBuffer.h"
#include "Passes/GBuffer/RasterizedGBuffer.h"
#include "Passes/GBuffer/SkyMotionVectors.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/FrameGraphTextureDesc.h"

#include <array>

class GBufferTargetFactory final
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

	static FrameGraphTextureHandle CreateGBufferDeviceZ(
	    FrameGraphBuilder& builder,
	    RenderViewportExtent sceneExtent,
	    GBufferMode gBufferMode)
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
	targets.BaseColor = GBufferTargetFactory::CreateGBufferColor(
	    builder,
	    "GBufferBaseColor",
	    sceneExtent,
	    GBufferFormats::BaseColor,
	    {0.0f, 0.0f, 0.0f, 1.0f});
	targets.Normal =
	    GBufferTargetFactory::CreateGBufferColor(builder, "GBufferNormal", sceneExtent, GBufferFormats::Normal, {0.0f, 0.0f, 1.0f, 0.0f});
	targets.Material = GBufferTargetFactory::CreateGBufferColor(
	    builder,
	    "GBufferMaterial",
	    sceneExtent,
	    GBufferFormats::Material,
	    {0.0f, 1.0f, 1.0f, 0.04f});
	targets.Emissive = GBufferTargetFactory::CreateGBufferColor(
	    builder,
	    "GBufferEmissive",
	    sceneExtent,
	    GBufferFormats::Emissive,
	    {0.0f, 0.0f, 0.0f, 0.0f});
	targets.Subsurface = GBufferTargetFactory::CreateGBufferColor(
	    builder,
	    "GBufferSubsurface",
	    sceneExtent,
	    GBufferFormats::Subsurface,
	    {0.0f, 0.0f, 0.0f, 0.0f});
	targets.MotionVector = GBufferTargetFactory::CreateGBufferColor(
	    builder,
	    "GBufferMotionVector",
	    sceneExtent,
	    GBufferFormats::MotionVector,
	    {0.0f, 0.0f, 0.0f, 0.0f});
	targets.DeviceZ = GBufferTargetFactory::CreateGBufferDeviceZ(builder, sceneExtent, gBufferMode);
	return targets;
}

void AddGBufferPasses(
    FrameGraphBuilder& builder,
    GpuMeshCache& gpuMeshCache,
    GBufferMode mode,
    RenderViewportExtent sceneExtent,
    RenderFrameGraphResources& resources)
{
	resources.Transient.GBuffer = CreateGBufferRenderTargets(builder, sceneExtent, mode);
	resources.ViewportProducts.Normals = resources.Transient.GBuffer.Normal;

	switch (mode)
	{
		case GBufferMode::Rasterized:
		default:
			AddRasterizedGBufferPass(builder, gpuMeshCache, resources.Transient.GBuffer, resources.ImportedScene);
			break;
		case GBufferMode::Raytraced:
			AddRaytracedGBufferPass(builder, sceneExtent, resources.Transient.GBuffer, resources.SceneTlas, resources.ImportedScene);
			break;
	}

	AddSkyMotionVectorPass(builder, sceneExtent, resources.Transient.GBuffer);
	AddLinearizeDeviceZPass(builder, sceneExtent, resources.Transient.GBuffer.DeviceZ, resources.Transient.Scene.SceneDepth);
}
