#include "PCH.h"
#include "Passes/GBuffer/GBuffer.h"

#include "Debug/RendererCVars.h"
#include "Passes/GBuffer/GBufferFormats.h"
#include "Passes/GBuffer/SceneDepth.h"
#include "Passes/GBuffer/RayTracingGBuffer.h"
#include "Passes/GBuffer/RasterizedGBuffer.h"
#include "Passes/GBuffer/SkyMotionVectors.h"
#include "Scene/RayTracing/RenderRayTracingScene.h"
#include "Core/Public/Diagnostics/Error.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/FrameGraphTextureDesc.h"

#include <array>

namespace GBufferTargetCreation
{
	FrameGraphTextureHandle CreateColor(
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

	FrameGraphTextureHandle CreateDeviceZ(
	    FrameGraphBuilder& builder,
	    RenderViewportExtent sceneExtent)
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
}

GBufferRenderTargets CreateGBufferRenderTargets(
	FrameGraphBuilder& builder,
	RenderViewportExtent sceneExtent)
{
	GBufferRenderTargets targets{};
	targets.BaseColor = GBufferTargetCreation::CreateColor(
	    builder,
	    "GBufferBaseColor",
	    sceneExtent,
	    GBufferFormats::BaseColor,
	    {0.0f, 0.0f, 0.0f, 1.0f});
	targets.Normal =
	    GBufferTargetCreation::CreateColor(builder, "GBufferNormal", sceneExtent, GBufferFormats::Normal, {0.0f, 0.0f, 1.0f, 0.0f});
	targets.Material = GBufferTargetCreation::CreateColor(
	    builder,
	    "GBufferMaterial",
	    sceneExtent,
	    GBufferFormats::Material,
	    {0.0f, 1.0f, 1.0f, 0.04f});
	targets.Emissive = GBufferTargetCreation::CreateColor(
	    builder,
	    "GBufferEmissive",
	    sceneExtent,
	    GBufferFormats::Emissive,
	    {0.0f, 0.0f, 0.0f, 0.0f});
	targets.Subsurface = GBufferTargetCreation::CreateColor(
	    builder,
	    "GBufferSubsurface",
	    sceneExtent,
	    GBufferFormats::Subsurface,
	    {0.0f, 0.0f, 0.0f, 0.0f});
	targets.MotionVector = GBufferTargetCreation::CreateColor(
	    builder,
	    "GBufferMotionVector",
	    sceneExtent,
	    GBufferFormats::MotionVector,
	    {0.0f, 0.0f, 0.0f, 0.0f});
	targets.DeviceZ = GBufferTargetCreation::CreateDeviceZ(builder, sceneExtent);
	return targets;
}

void AddGBufferMeshPasses(
    FrameGraphBuilder& builder,
    GpuMeshCache& gpuMeshCache,
    const RenderRayTracingScene& rayTracingScene,
    bool hasMaskedRayTracingGeometry,
    RenderViewportExtent sceneExtent,
	RenderFrameGraphResources& resources)
{
	resources.Transient.GBuffer = CreateGBufferRenderTargets(builder, sceneExtent);
	resources.ViewportProducts.Normals = resources.Transient.GBuffer.Normal;

	switch (CVarGBufferAlgorithm.Get())
	{
		case GBufferAlgorithm::Rasterized:
			AddRasterizedGBufferMeshPass(builder, gpuMeshCache, resources.Transient.GBuffer, resources.ImportedScene);
			break;
		case GBufferAlgorithm::RayTracing:
		{
			AddRayTracingGBufferMeshPass(
			    builder,
			    sceneExtent,
			    resources.Transient.GBuffer,
			    resources.SceneTlas,
			    resources.ImportedScene,
			    hasMaskedRayTracingGeometry,
			    rayTracingScene.GetCapabilityReport());
			break;
		}
		default:
			throw Diagnostics::Error("GBuffer graph construction received an invalid algorithm.");
	}

	AddSkyMotionVectorPass(builder, sceneExtent, resources.Transient.GBuffer);
	AddLinearizeDeviceZPass(builder, sceneExtent, resources.Transient.GBuffer.DeviceZ, resources.Transient.Scene.SceneDepth);
}
