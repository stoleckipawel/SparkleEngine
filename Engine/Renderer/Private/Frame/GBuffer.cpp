#include "../PCH.h"
#include "Frame/GBuffer.h"

#include "Config/RenderConfig.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/GBufferPass.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureDesc.h"

GBufferRenderTargets CreateGBufferRenderTargets(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent, const SceneRenderTargets& sceneTargets)
{
	GBufferRenderTargets targets{};
	targets.BaseColor = builder.CreateTexture(
	    FrameGraphTextureDesc::CreateColor(
	        "GBufferBaseColor",
	        sceneExtent.Width,
	        sceneExtent.Height,
	        RenderConfig::GBuffer::BaseColorFormat));
	targets.Normal = builder.CreateTexture(
	    FrameGraphTextureDesc::CreateColor("GBufferNormal", sceneExtent.Width, sceneExtent.Height, RenderConfig::GBuffer::NormalFormat));
	targets.Material = builder.CreateTexture(
	    FrameGraphTextureDesc::CreateColor(
	        "GBufferMaterial",
	        sceneExtent.Width,
	        sceneExtent.Height,
	        RenderConfig::GBuffer::MaterialFormat));
	targets.Emissive = builder.CreateTexture(
	    FrameGraphTextureDesc::CreateColor(
	        "GBufferEmissive",
	        sceneExtent.Width,
	        sceneExtent.Height,
	        RenderConfig::GBuffer::EmissiveFormat));
	targets.Subsurface = builder.CreateTexture(
	    FrameGraphTextureDesc::CreateColor(
	        "GBufferSubsurface",
	        sceneExtent.Width,
	        sceneExtent.Height,
	        RenderConfig::GBuffer::SubsurfaceFormat));
	targets.MotionVector = builder.CreateTexture(
	    FrameGraphTextureDesc::CreateColor(
	        "GBufferMotionVector",
	        sceneExtent.Width,
	        sceneExtent.Height,
	        RenderConfig::GBuffer::MotionVectorFormat));
	targets.DeviceZ = builder.CreateTexture(
	    FrameGraphTextureDesc::CreateDepth("GBufferDeviceZ", sceneExtent.Width, sceneExtent.Height, RenderConfig::GBuffer::DeviceZFormat));
	targets.MainDepth = sceneTargets.MainDepth;
	return targets;
}

void AddGBufferPass(FrameGraphBuilder& builder, const GBufferRenderTargets& targets)
{
	auto& parameters = builder.AllocPassParameters<GBufferPass>();
	GBufferPass::DeclareResources(builder, targets, parameters);
	builder.AddRasterPass<GBufferPass>(
	    GBufferPass::PassName,
	    parameters,
	    [](PassExecutionContext& context, GBufferPass::ParameterInstance& passParameters)
	    {
		    const GBufferPass pass(context.RuntimeServices.GetPassRuntime<GBufferPass>());
		    pass.Execute(context, passParameters);
	    });
}
