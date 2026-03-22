#include "PCH.h"

#include "FrameGraph/Builder/ShadowFrameGraphBuilder.h"

#include "Renderer/Public/FrameContext.h"
#include "Renderer/Public/FrameGraph/FrameGraph.h"
#include "Renderer/Public/Passes/ShadowOpaquePass.h"

#include <memory>
#include <string>

ShadowFrameGraphBuilder::ShadowFrameGraphBuilder(
	D3D12RootSignature& rootSignature,
	D3D12PipelineState& shadowPipelineState,
	D3D12ConstantBufferManager& constantBufferManager) noexcept :
	m_rootSignature(&rootSignature),
	m_shadowPipelineState(&shadowPipelineState),
	m_constantBufferManager(&constantBufferManager)
{
}

ShadowFrameGraphResources ShadowFrameGraphBuilder::Build(FrameGraph& frameGraph) const
{
	ShadowFrameGraphResources resources{};

	for (std::size_t i = 0; i < ShadowFrameGraphResources::MaxShadowMaps; ++i)
	{
		const std::string mapName = "ShadowMap" + std::to_string(i);
		const std::string depthName = "ShadowDepth" + std::to_string(i);
		const std::string passName = "ShadowPass" + std::to_string(i);

		resources.shadowMapHandles[i] = frameGraph.CreateTexture(FrameGraphTextureDesc::CreateColor(
		    mapName.c_str(),
		    RenderConfig::Shadows::ShadowMapResolution,
		    RenderConfig::Shadows::ShadowMapResolution,
		    RenderConfig::Shadows::ShadowMapFormat));
		TextureHandle shadowDepthHandle = frameGraph.CreateTexture(FrameGraphTextureDesc::CreateDepthStencil(
		    depthName.c_str(),
		    RenderConfig::Shadows::ShadowMapResolution,
		    RenderConfig::Shadows::ShadowMapResolution));

		auto shadowPass = std::make_shared<ShadowOpaquePass>(
		    *m_rootSignature,
		    *m_shadowPipelineState,
		    *m_constantBufferManager,
		    resources.shadowMapHandles[i],
		    shadowDepthHandle);

		const std::size_t lightIndex = i;
		frameGraph.AddPass(
		    passName.c_str(),
		    FrameGraphPassFlags::Raster,
		    [shadowColor = resources.shadowMapHandles[i], shadowDepth = shadowDepthHandle](PassBuilder& builder)
		    {
			    builder.Write(shadowColor, ResourceUsage::RenderTarget);
			    builder.Write(shadowDepth, ResourceUsage::DepthWrite);
		    },
		    [shadowPass, lightIndex](const FrameGraph& graph, CommandContext& cmd, const FrameContext& frame)
		    {
			    if (lightIndex < frame.shadowViewCount)
			    {
				    shadowPass->Execute(graph, cmd, frame.sceneData, frame.shadowViews[lightIndex]);
			    }
		    });
	}

	return resources;
}