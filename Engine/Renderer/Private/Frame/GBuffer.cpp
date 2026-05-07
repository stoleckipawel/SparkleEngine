#include "../PCH.h"
#include "Frame/GBuffer.h"

#include "Config/RenderConfig.h"
#include "FrameGraph/FrameGraph.h"
#include "Passes/GBufferPass.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureDesc.h"

GBufferTargets BuildGBuffer(
    FrameGraph& frameGraph,
    RenderViewportExtent sceneExtent,
    const SceneTargets& sceneTargets)
{
	GBufferTargets targets{};
	targets.BaseColor = frameGraph.CreateTexture(
	    FrameGraphTextureDesc::CreateColor("GBufferBaseColor", sceneExtent.Width, sceneExtent.Height, RenderConfig::GBuffer::BaseColorFormat));
	targets.Normal = frameGraph.CreateTexture(
	    FrameGraphTextureDesc::CreateColor("GBufferNormal", sceneExtent.Width, sceneExtent.Height, RenderConfig::GBuffer::NormalFormat));
	targets.Material = frameGraph.CreateTexture(
	    FrameGraphTextureDesc::CreateColor("GBufferMaterial", sceneExtent.Width, sceneExtent.Height, RenderConfig::GBuffer::MaterialFormat));
	targets.Emissive = frameGraph.CreateTexture(
	    FrameGraphTextureDesc::CreateColor("GBufferEmissive", sceneExtent.Width, sceneExtent.Height, RenderConfig::GBuffer::EmissiveFormat));
	targets.Subsurface = frameGraph.CreateTexture(
	    FrameGraphTextureDesc::CreateColor("GBufferSubsurface", sceneExtent.Width, sceneExtent.Height, RenderConfig::GBuffer::SubsurfaceFormat));
	targets.DeviceZ = frameGraph.CreateTexture(
	    FrameGraphTextureDesc::CreateDepth("GBufferDeviceZ", sceneExtent.Width, sceneExtent.Height, RenderConfig::GBuffer::DeviceZFormat));
	targets.MainDepth = sceneTargets.MainDepth;

	auto& parameters = frameGraph.AllocPassParameters<GBufferPass>();
	GBufferPass::DeclareResources(frameGraph, targets, parameters);
	frameGraph.AddRasterPass<GBufferPass>(GBufferPass::PassName, parameters);
	return targets;
}
