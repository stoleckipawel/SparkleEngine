#include "PCH.h"

#include "FrameGraph/Builder/ShadowFrameGraphBuilder.h"

#include "Renderer/Public/FrameContext.h"
#include "Renderer/Public/FrameGraph/FrameGraph.h"
#include "Renderer/Public/Passes/ShadowOpaquePass.h"

#include <memory>

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

	resources.shadowMapHandle = frameGraph.CreateTexture(FrameGraphTextureDesc::CreateColor(
	    "ShadowMap",
	    RenderConfig::Shadows::ShadowMapResolution,
	    RenderConfig::Shadows::ShadowMapResolution,
	    RenderConfig::Shadows::ShadowMapFormat));
	TextureHandle shadowDepthHandle = frameGraph.CreateTexture(FrameGraphTextureDesc::CreateDepthStencil(
	    "ShadowDepth",
	    RenderConfig::Shadows::ShadowMapResolution,
	    RenderConfig::Shadows::ShadowMapResolution));

	auto shadowPass = std::make_shared<ShadowOpaquePass>(
	    *m_rootSignature,
	    *m_shadowPipelineState,
	    *m_constantBufferManager,
	    resources.shadowMapHandle,
	    shadowDepthHandle);

	frameGraph.AddPass(
	    "ShadowPass",
	    FrameGraphPassFlags::Raster,
	    [shadowColor = resources.shadowMapHandle, shadowDepth = shadowDepthHandle](PassBuilder& builder)
	    {
		    builder.Write(shadowColor, ResourceUsage::RenderTarget);
		    builder.Write(shadowDepth, ResourceUsage::DepthWrite);
	    },
	    [shadowPass](const FrameGraph& graph, CommandContext& cmd, const FrameContext& frame)
	    {
		    shadowPass->Execute(graph, cmd, frame.sceneData, frame.shadowView);
	    });

	return resources;
}