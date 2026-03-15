#include "PCH.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"

#include "Renderer/Public/FrameGraph/FrameGraph.h"
#include "Renderer/Public/Passes/ForwardOpaquePass.h"

#include "UI.h"
#include "Window.h"

FrameGraphBuilder::FrameGraphBuilder(const FrameGraphDependencies& dependencies) noexcept
 : m_dependencies(dependencies) 
{
    
}

std::unique_ptr<FrameGraph> FrameGraphBuilder::Build() const
{
	auto frameGraph = std::make_unique<FrameGraph>(
	    &m_dependencies.rhi,
	    &m_dependencies.window,
	    &m_dependencies.descriptorHeapManager,
	    &m_dependencies.swapChain);

	const FrameGraphTextureDesc backBufferDesc = FrameGraphTextureDesc::CreateColor(
	    "BackBuffer",
	    static_cast<std::uint32_t>(m_dependencies.window.GetWidth()),
	    static_cast<std::uint32_t>(m_dependencies.window.GetHeight()),
	    RHISettings::BackBufferFormat);
	const TextureHandle backBufferHandle = frameGraph->ImportTexture(backBufferDesc, ResourceState::Present);

	const FrameGraphTextureDesc mainDepthDesc = FrameGraphTextureDesc::CreateDepthStencil(
	    "MainDepth",
	    static_cast<std::uint32_t>(m_dependencies.window.GetWidth()),
	    static_cast<std::uint32_t>(m_dependencies.window.GetHeight()));
	const TextureHandle mainDepthHandle = frameGraph->CreateTexture(mainDepthDesc);

	auto forwardOpaquePass = std::make_shared<ForwardOpaquePass>(
	    m_dependencies.rootSignature,
	    m_dependencies.pipelineState,
	    m_dependencies.constantBufferManager,
	    m_dependencies.descriptorHeapManager,
	    m_dependencies.textureManager,
	    m_dependencies.samplerLibrary,
	    m_dependencies.gpuMeshCache,
	    backBufferHandle,
	    mainDepthHandle);

	frameGraph->AddPass(
	    "ForwardOpaque",
	    FrameGraphPassFlags::Raster,
	    [backBufferHandle, mainDepthHandle](PassBuilder& builder)
	    {
		    builder.Write(backBufferHandle, ResourceUsage::RenderTarget);
		    builder.Write(mainDepthHandle, ResourceUsage::DepthWrite);
	    },
	    [forwardOpaquePass](const FrameGraph& frameGraph, CommandContext& cmd, const FrameContext& frame)
	    {
		    forwardOpaquePass->Execute(frameGraph, cmd, frame);
	    });

	frameGraph->AddPass(
	    "UIComposition",
	    FrameGraphPassFlags::Raster,
	    [backBufferHandle](PassBuilder& builder)
	    {
		    builder.Write(backBufferHandle, ResourceUsage::RenderTarget);
	    },
	    [ui = &m_dependencies.ui, backBufferHandle](const FrameGraph& frameGraph, CommandContext& cmd, const FrameContext& frame)
	    {
		    (void) frame;
		    frameGraph.BindRenderTarget(cmd, backBufferHandle);
		    ui->Render(cmd);
	    });

	return frameGraph;
}