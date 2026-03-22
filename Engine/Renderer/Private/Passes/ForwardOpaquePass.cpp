#include "PCH.h"
#include "Renderer/Public/Passes/ForwardOpaquePass.h"

#include "Renderer/Public/CommandContext.h"
#include "Renderer/Public/Frame/RenderViewContext.h"
#include "Renderer/Public/FrameGraph/FrameGraph.h"
#include "Renderer/Public/SceneData/RenderSceneData.h"
#include "Renderer/Public/SceneData/MeshDraw.h"
#include "Renderer/Public/GPU/GPUMesh.h"
#include "Renderer/Public/TextureManager.h"

#include "D3D12RootSignature.h"
#include "D3D12PipelineState.h"
#include "D3D12ConstantBufferManager.h"
#include "D3D12ConstantBufferData.h"
#include "D3D12RootBindings.h"
#include "D3D12DescriptorHeapManager.h"
#include "D3D12Texture.h"
#include "Samplers/D3D12SamplerLibrary.h"

#include "Core/Public/Diagnostics/Log.h"

ForwardOpaquePass::ForwardOpaquePass(
    D3D12RootSignature& rootSignature,
    D3D12PipelineState& pipelineState,
    D3D12ConstantBufferManager& constantBufferManager,
    D3D12DescriptorHeapManager& descriptorHeapManager,
    TextureManager& textureManager,
    D3D12SamplerLibrary& samplerLibrary,
	std::array<TextureHandle, ForwardOpaquePass::MaxShadowMaps> shadowMapHandles,
    TextureHandle backBufferHandle,
    TextureHandle depthBufferHandle) noexcept :
    m_rootSignature(&rootSignature),
    m_pipelineState(&pipelineState),
    m_constantBufferManager(&constantBufferManager),
    m_descriptorHeapManager(&descriptorHeapManager),
    m_textureManager(&textureManager),
    m_samplerLibrary(&samplerLibrary),
	m_shadowMaps(shadowMapHandles),
    m_backBuffer(backBufferHandle),
    m_depthBuffer(depthBufferHandle)
{
	LOG_INFO("ForwardOpaquePass: Created");
}

void ForwardOpaquePass::Execute(
	const FrameGraph& frameGraph,
	CommandContext& cmd,
	const RenderSceneData& sceneData,
	const RenderViewContext& viewContext)
{
	PrepareTargets(frameGraph, cmd);
	ConfigurePipeline(cmd, viewContext);
	BindFrameResources(cmd, viewContext);
	BindGlobalResources(frameGraph, cmd);
	DrawOpaqueMeshes(cmd, sceneData);
}

void ForwardOpaquePass::PrepareTargets(const FrameGraph& frameGraph, CommandContext& cmd)
{
	frameGraph.BindRenderTarget(cmd, m_backBuffer, m_depthBuffer);
	frameGraph.ClearRenderTarget(cmd, m_backBuffer);
	frameGraph.ClearDepthStencil(cmd, m_depthBuffer);
}

void ForwardOpaquePass::ConfigurePipeline(CommandContext& cmd, const RenderViewContext& viewContext)
{
	cmd.SetRootSignature(m_rootSignature->GetRaw());

	const D3D12_VIEWPORT viewport = viewContext.viewport;
	cmd.SetViewport(viewport.TopLeftX, viewport.TopLeftY, viewport.Width, viewport.Height, viewport.MinDepth, viewport.MaxDepth);

	const D3D12_RECT scissor = viewContext.scissorRect;
	cmd.SetScissorRect(scissor.left, scissor.top, scissor.right, scissor.bottom);

	cmd.SetPipelineState(m_pipelineState->Get().Get());
	cmd.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void ForwardOpaquePass::BindFrameResources(CommandContext& cmd, const RenderViewContext& viewContext)
{
	cmd.BindConstantBuffer(RootBindings::RootParam::PerFrame, m_constantBufferManager->GetPerFrameGpuAddress());
	cmd.BindConstantBuffer(RootBindings::RootParam::PerView, viewContext.perViewGpuAddress);
}

void ForwardOpaquePass::BindGlobalResources(const FrameGraph& frameGraph, CommandContext& cmd)
{
	m_descriptorHeapManager->SetShaderVisibleHeaps(cmd);

	const D3D12_GPU_DESCRIPTOR_HANDLE shadowMap0Srv = frameGraph.ResolveShaderResourceView(m_shadowMaps[0]);
	cmd.BindDescriptorTable(RootBindings::RootParam::ShadowMap0, shadowMap0Srv);

	const D3D12_GPU_DESCRIPTOR_HANDLE shadowMap1Srv = frameGraph.ResolveShaderResourceView(m_shadowMaps[1]);
	cmd.BindDescriptorTable(RootBindings::RootParam::ShadowMap1, shadowMap1Srv);

	const D3D12_GPU_DESCRIPTOR_HANDLE shadowMap2Srv = frameGraph.ResolveShaderResourceView(m_shadowMaps[2]);
	cmd.BindDescriptorTable(RootBindings::RootParam::ShadowMap2, shadowMap2Srv);

	const D3D12_GPU_DESCRIPTOR_HANDLE shadowMap3Srv = frameGraph.ResolveShaderResourceView(m_shadowMaps[3]);
	cmd.BindDescriptorTable(RootBindings::RootParam::ShadowMap3, shadowMap3Srv);

	if (m_samplerLibrary->IsInitialized())
	{
		cmd.BindDescriptorTable(RootBindings::RootParam::SamplerTable, m_samplerLibrary->GetTableGPUHandle());
	}
}

void ForwardOpaquePass::DrawOpaqueMeshes(CommandContext& cmd, const RenderSceneData& sceneData)
{
	for (const auto& draw : sceneData.meshDraws)
	{
		const GPUMesh* gpuMesh = draw.gpuMesh;

		if (!gpuMesh || !gpuMesh->IsValid())
		{
			continue;
		}

		cmd.BindVertexBuffer(gpuMesh->GetVertexBufferView());
		cmd.BindIndexBuffer(gpuMesh->GetIndexBufferView());

		PerObjectVSConstantBufferData perObjectVS{};
		perObjectVS.WorldMTX = draw.worldMatrix;
		perObjectVS.WorldInvTransposeMTX = draw.worldInvTranspose;

		cmd.BindConstantBuffer(RootBindings::RootParam::PerObjectVS, m_constantBufferManager->UpdatePerObjectVS(perObjectVS));

		cmd.BindConstantBuffer(
		    RootBindings::RootParam::PerObjectPS,
		    m_constantBufferManager->UpdatePerObjectPS(sceneData.materials[draw.materialId].ToPerObjectPSData()));

		const D3D12_GPU_DESCRIPTOR_HANDLE materialTextureTable = sceneData.materials[draw.materialId].textureTableGpuHandle;
		if (materialTextureTable.ptr == 0)
		{
			LOG_WARNING("ForwardOpaquePass::DrawOpaqueMeshes: Material texture table is invalid; draw skipped.");
			continue;
		}

		cmd.BindDescriptorTable(RootBindings::RootParam::TextureSRV, materialTextureTable);

		cmd.DrawIndexedInstanced(gpuMesh->GetIndexCount(), 1, 0, 0, 0);
	}
}
