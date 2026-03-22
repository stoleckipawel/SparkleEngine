#include "PCH.h"

#include "Renderer/Public/Passes/ShadowOpaquePass.h"

#include "Renderer/Public/CommandContext.h"
#include "Renderer/Public/Frame/RenderViewContext.h"
#include "Renderer/Public/FrameGraph/FrameGraph.h"
#include "Renderer/Public/GPU/GPUMesh.h"
#include "Renderer/Public/SceneData/MeshDraw.h"
#include "Renderer/Public/SceneData/RenderSceneData.h"

#include "D3D12ConstantBufferData.h"
#include "D3D12ConstantBufferManager.h"
#include "D3D12PipelineState.h"
#include "D3D12RootBindings.h"
#include "D3D12RootSignature.h"

ShadowOpaquePass::ShadowOpaquePass(
	D3D12RootSignature& rootSignature,
	D3D12PipelineState& pipelineState,
	D3D12ConstantBufferManager& constantBufferManager,
	TextureHandle shadowMapHandle,
	TextureHandle depthBufferHandle) noexcept :
	m_rootSignature(&rootSignature),
	m_pipelineState(&pipelineState),
	m_constantBufferManager(&constantBufferManager),
	m_shadowMap(shadowMapHandle),
	m_depthBuffer(depthBufferHandle)
{
}

void ShadowOpaquePass::Execute(
	const FrameGraph& frameGraph,
	CommandContext& cmd,
	const RenderSceneData& sceneData,
	const RenderViewContext& viewContext)
{
	PrepareTargets(frameGraph, cmd);
	ConfigurePipeline(cmd, viewContext);
	BindFrameResources(cmd, viewContext);
	DrawMeshes(cmd, sceneData);
}

void ShadowOpaquePass::PrepareTargets(const FrameGraph& frameGraph, CommandContext& cmd)
{
	frameGraph.BindRenderTarget(cmd, m_shadowMap, m_depthBuffer);
	frameGraph.ClearRenderTarget(cmd, m_shadowMap);
	frameGraph.ClearDepthStencil(cmd, m_depthBuffer);
}

void ShadowOpaquePass::ConfigurePipeline(CommandContext& cmd, const RenderViewContext& viewContext)
{
	cmd.SetRootSignature(m_rootSignature->GetRaw());
	cmd.SetViewport(
	    viewContext.viewport.TopLeftX,
	    viewContext.viewport.TopLeftY,
	    viewContext.viewport.Width,
	    viewContext.viewport.Height,
	    viewContext.viewport.MinDepth,
	    viewContext.viewport.MaxDepth);
	cmd.SetScissorRect(
	    viewContext.scissorRect.left,
	    viewContext.scissorRect.top,
	    viewContext.scissorRect.right,
	    viewContext.scissorRect.bottom);
	cmd.SetPipelineState(m_pipelineState->Get().Get());
	cmd.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void ShadowOpaquePass::BindFrameResources(CommandContext& cmd, const RenderViewContext& viewContext)
{
	cmd.BindConstantBuffer(RootBindings::RootParam::PerFrame, m_constantBufferManager->GetPerFrameGpuAddress());
	cmd.BindConstantBuffer(RootBindings::RootParam::PerView, viewContext.perViewGpuAddress);
}

void ShadowOpaquePass::DrawMeshes(CommandContext& cmd, const RenderSceneData& sceneData)
{
	for (const MeshDraw& draw : sceneData.meshDraws)
	{
		const GPUMesh* gpuMesh = draw.gpuMesh;
		if (gpuMesh == nullptr || !gpuMesh->IsValid())
		{
			continue;
		}

		cmd.BindVertexBuffer(gpuMesh->GetVertexBufferView());
		cmd.BindIndexBuffer(gpuMesh->GetIndexBufferView());

		PerObjectVSConstantBufferData perObjectVS{};
		perObjectVS.WorldMTX = draw.worldMatrix;
		perObjectVS.WorldInvTransposeMTX = draw.worldInvTranspose;
		cmd.BindConstantBuffer(RootBindings::RootParam::PerObjectVS, m_constantBufferManager->UpdatePerObjectVS(perObjectVS));

		cmd.DrawIndexedInstanced(gpuMesh->GetIndexCount(), 1, 0, 0, 0);
	}
}