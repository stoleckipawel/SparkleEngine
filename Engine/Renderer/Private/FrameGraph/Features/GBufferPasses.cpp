#include "../../PCH.h"

#include "FrameGraph/Features/GBufferPasses.h"

#include "Config/RenderConfig.h"
#include "FrameGraph/FrameGraph.h"
#include "Passes/GBufferPass.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureDesc.h"
#include "RHI/Public/Interop/RenderHardwareInterface.h"
#include "Window/Window.h"

namespace FrameGraphFeatures
{
	FrameGraphGBufferTargets AddGBufferPass(
	    FrameGraph& frameGraph,
	    const Window& window,
	    const RenderViewportExtent& sceneExtent,
	    const FrameGraphSceneTargets& sceneTargets)
	{
		const std::uint32_t width = static_cast<std::uint32_t>(window.GetWidth());
		const std::uint32_t height = static_cast<std::uint32_t>(window.GetHeight());
		const std::uint32_t sceneWidth = sceneExtent.IsValid() ? sceneExtent.Width : width;
		const std::uint32_t sceneHeight = sceneExtent.IsValid() ? sceneExtent.Height : height;

		FrameGraphGBufferTargets targets{};
		targets.BaseColor = frameGraph.CreateTexture(FrameGraphTextureDesc::CreateColor(
		    "GBufferBaseColor",
		    sceneWidth,
		    sceneHeight,
		    RenderConfig::GBuffer::BaseColorFormat));
		targets.Normal = frameGraph.CreateTexture(FrameGraphTextureDesc::CreateColor(
		    "GBufferNormal",
		    sceneWidth,
		    sceneHeight,
		    RenderConfig::GBuffer::NormalFormat));
		targets.Material = frameGraph.CreateTexture(FrameGraphTextureDesc::CreateColor(
		    "GBufferMaterial",
		    sceneWidth,
		    sceneHeight,
		    RenderConfig::GBuffer::MaterialFormat));
		targets.Emissive = frameGraph.CreateTexture(FrameGraphTextureDesc::CreateColor(
		    "GBufferEmissive",
		    sceneWidth,
		    sceneHeight,
		    RenderConfig::GBuffer::EmissiveFormat));
		targets.MainDepth = sceneTargets.MainDepth;

		auto& parameters = frameGraph.AllocPassParameters<GBufferPass>();
		parameters->BaseColor = frameGraph.CreateRenderTarget(targets.BaseColor);
		parameters->Normal = frameGraph.CreateRenderTarget(targets.Normal);
		parameters->Material = frameGraph.CreateRenderTarget(targets.Material);
		parameters->Emissive = frameGraph.CreateRenderTarget(targets.Emissive);
		parameters->MainDepth = frameGraph.CreateDepthTarget(targets.MainDepth);
		parameters->SamplerAniso16xWrap = RhiSamplerDesc{.MaxAnisotropy = RhiSamplerAnisotropy::X16};

		frameGraph.AddRasterPass<GBufferPass>(GBufferPass::PassName, parameters);
		return targets;
	}
}  // namespace FrameGraphFeatures
