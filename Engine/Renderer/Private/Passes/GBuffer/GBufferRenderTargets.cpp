#include "PCH.h"
#include "Passes/GBuffer/GBufferRenderTargets.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Debug/RendererCVars.h"
#include "Frame/Graph/RenderFrameGraphResources.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/FrameGraphTextureDesc.h"
#include "Passes/GBuffer/GBufferFormats.h"

#include <array>

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

static FrameGraphTextureHandle CreateGBufferDeviceZ(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent)
{
	switch (CVarGBufferAlgorithm.Get())
	{
		case GBufferAlgorithm::RayTracing:
			return builder.CreateTexture(
			    FrameGraphTextureDesc::CreateColor(
			        "GBufferDeviceZ",
			        sceneExtent.Width,
			        sceneExtent.Height,
			        GBufferFormats::RayTracingDeviceZ));
		case GBufferAlgorithm::Rasterized:
			return builder.CreateTexture(
			    FrameGraphTextureDesc::CreateDepthTarget(
			        "GBufferDeviceZ",
			        sceneExtent.Width,
			        sceneExtent.Height,
			        GBufferFormats::RasterizedDeviceZ));
		default:
			throw Diagnostics::Error("GBuffer target creation received an invalid algorithm.");
	}
}

void CreateGBufferRenderTargets(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    RenderFrameGraphResources& resources)
{
	GBufferRenderTargets& targets = resources.Transient.GBuffer;
	targets.BaseColor =
	    CreateGBufferColor(builder, "GBufferBaseColor", sceneExtent, GBufferFormats::BaseColor, {0.0f, 0.0f, 0.0f, 1.0f});
	targets.Normal = CreateGBufferColor(builder, "GBufferNormal", sceneExtent, GBufferFormats::Normal, {0.0f, 0.0f, 1.0f, 0.0f});
	targets.Material =
	    CreateGBufferColor(builder, "GBufferMaterial", sceneExtent, GBufferFormats::Material, {0.0f, 1.0f, 1.0f, 0.04f});
	targets.Emissive =
	    CreateGBufferColor(builder, "GBufferEmissive", sceneExtent, GBufferFormats::Emissive, {0.0f, 0.0f, 0.0f, 0.0f});
	targets.Subsurface =
	    CreateGBufferColor(builder, "GBufferSubsurface", sceneExtent, GBufferFormats::Subsurface, {0.0f, 0.0f, 0.0f, 0.0f});
	targets.MotionVector =
	    CreateGBufferColor(builder, "GBufferMotionVector", sceneExtent, GBufferFormats::MotionVector, {0.0f, 0.0f, 0.0f, 0.0f});
	targets.DeviceZ = CreateGBufferDeviceZ(builder, sceneExtent);
	resources.ViewportProducts.Normals = targets.Normal;
}
