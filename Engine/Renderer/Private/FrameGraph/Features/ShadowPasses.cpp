#include "../../PCH.h"

#include "FrameGraph/Features/ShadowPasses.h"

#include "FrameGraph/FrameGraph.h"
#include "Passes/ShadowOpaquePass.h"

#include <string>

namespace FrameGraphFeatures
{
	FrameGraphShadowOutputs AddShadowPasses(FrameGraph& frameGraph)
	{
		FrameGraphShadowOutputs outputs{};

		for (std::size_t lightIndex = 0; lightIndex < FrameGraphShadowOutputs::MaxShadowMaps; ++lightIndex)
		{
			const std::string mapName = "ShadowMap" + std::to_string(lightIndex);
			const std::string depthName = "ShadowDepth" + std::to_string(lightIndex);
			const std::string passName = "ShadowPass" + std::to_string(lightIndex);

			outputs.ShadowMaps[lightIndex] = frameGraph.CreateTexture(
			    FrameGraphTextureDesc::CreateColor(
			        mapName.c_str(),
			        RenderConfig::Shadows::ShadowMapResolution,
			        RenderConfig::Shadows::ShadowMapResolution,
			        RenderConfig::Shadows::ShadowMapFormat));
			const TextureHandle shadowDepthHandle = frameGraph.CreateTexture(
			    FrameGraphTextureDesc::CreateDepthStencil(
			        depthName.c_str(),
			        RenderConfig::Shadows::ShadowMapResolution,
			        RenderConfig::Shadows::ShadowMapResolution));

			auto& shadowParameters = frameGraph.AllocPassParameters<ShadowOpaquePass>();
			shadowParameters->ShadowColor = frameGraph.CreateRenderTarget(outputs.ShadowMaps[lightIndex]);
			shadowParameters->ShadowDepth = frameGraph.CreateDepthTarget(shadowDepthHandle);

			frameGraph.AddRasterPass<ShadowOpaquePass>(passName, shadowParameters, lightIndex);
		}

		return outputs;
	}
}  // namespace FrameGraphFeatures
