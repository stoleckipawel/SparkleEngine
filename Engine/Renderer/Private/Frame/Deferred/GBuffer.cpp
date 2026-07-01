#include "PCH.h"
#include "Frame/Deferred/GBuffer.h"

#include "Frame/Deferred/GBufferFormats.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Deferred/GBufferPass.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureDesc.h"

GBufferRenderTargets CreateGBufferRenderTargets(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent, const SceneRenderTargets& sceneTargets)
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
	targets.DeviceZ = builder.CreateTexture(
	    FrameGraphTextureDesc::CreateDepth("GBufferDeviceZ", sceneExtent.Width, sceneExtent.Height, GBufferFormats::DeviceZ));
	targets.MainDepth = sceneTargets.MainDepth;
	return targets;
}

void AddGBufferPass(FrameGraphBuilder& builder, const GBufferRenderTargets& targets)
{
	auto& parameters = builder.AllocPassParameters<GBufferPass>();
	GBufferPass::DeclareResources(builder, targets, parameters);
	builder.AddRasterShaderPass<GBufferPass>(parameters);
}

void AddGBufferPasses(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent, FrameAssemblyResourceLayout& resources)
{
	resources.Transient.GBuffer = CreateGBufferRenderTargets(builder, sceneExtent, resources.Transient.Scene);
	resources.ViewportProducts.SceneDepth = resources.Transient.Scene.MainDepth;
	resources.ViewportProducts.Normals = resources.Transient.GBuffer.Normal;
	resources.ViewportProducts.MotionVectors = resources.Transient.GBuffer.MotionVector;
	AddGBufferPass(builder, resources.Transient.GBuffer);
}
