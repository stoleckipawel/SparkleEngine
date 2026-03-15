#include "PCH.h"
#include "Renderer/Public/Passes/ForwardOpaquePass.h"

#include "Renderer/Public/CommandContext.h"
#include "Renderer/Public/FrameContext.h"
#include "Renderer/Public/FrameGraph/FrameGraph.h"
#include "Renderer/Public/SceneData/MeshDraw.h"
#include "Renderer/Public/GPU/GPUMesh.h"
#include "Renderer/Public/GPU/GPUMeshCache.h"
#include "Renderer/Public/TextureManager.h"

#include "D3D12RootSignature.h"
#include "D3D12PipelineState.h"
#include "D3D12ConstantBufferManager.h"
#include "D3D12ConstantBufferData.h"
#include "D3D12RootBindings.h"
#include "D3D12DescriptorHeapManager.h"
#include "D3D12Texture.h"
#include "Samplers/D3D12SamplerLibrary.h"
#include "Scene/Mesh.h"

#include "Core/Public/Diagnostics/Log.h"

ForwardOpaquePass::ForwardOpaquePass(
    D3D12RootSignature& rootSignature,
    D3D12PipelineState& pipelineState,
    D3D12ConstantBufferManager& constantBufferManager,
    D3D12DescriptorHeapManager& descriptorHeapManager,
    TextureManager& textureManager,
    D3D12SamplerLibrary& samplerLibrary,
    GPUMeshCache& gpuMeshCache,
	TextureHandle backBufferHandle,
	TextureHandle depthBufferHandle) noexcept :
    m_rootSignature(&rootSignature),
    m_pipelineState(&pipelineState),
    m_constantBufferManager(&constantBufferManager),
    m_descriptorHeapManager(&descriptorHeapManager),
    m_textureManager(&textureManager),
    m_samplerLibrary(&samplerLibrary),
    m_gpuMeshCache(&gpuMeshCache),
	m_backBuffer(backBufferHandle),
	m_depthBuffer(depthBufferHandle)
{
	LOG_INFO("ForwardOpaquePass: Created");
}

void ForwardOpaquePass::Execute(const FrameGraph& frameGraph, CommandContext& cmd, const FrameContext& frame)
{
	PrepareTargets(frameGraph, cmd);
	ConfigurePipeline(cmd, frame);
	BindFrameResources(cmd);
	BindGlobalResources(cmd);
	DrawOpaqueMeshes(cmd, frame);
}

void ForwardOpaquePass::PrepareTargets(const FrameGraph& frameGraph, CommandContext& cmd)
{
	frameGraph.BindRenderTarget(cmd, m_backBuffer, m_depthBuffer);
	frameGraph.ClearRenderTarget(cmd, m_backBuffer);
	frameGraph.ClearDepthStencil(cmd, m_depthBuffer);
}

void ForwardOpaquePass::ConfigurePipeline(CommandContext& cmd, const FrameContext& frame)
{
	cmd.SetRootSignature(m_rootSignature->GetRaw());

	const D3D12_VIEWPORT viewport = frame.viewport;
	cmd.SetViewport(viewport.TopLeftX, viewport.TopLeftY, viewport.Width, viewport.Height, viewport.MinDepth, viewport.MaxDepth);

	const D3D12_RECT scissor = frame.scissorRect;
	cmd.SetScissorRect(scissor.left, scissor.top, scissor.right, scissor.bottom);

	cmd.SetPipelineState(m_pipelineState->Get().Get());
	cmd.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void ForwardOpaquePass::BindFrameResources(CommandContext& cmd)
{
	cmd.BindConstantBuffer(RootBindings::RootParam::PerFrame, m_constantBufferManager->GetPerFrameGpuAddress());

	cmd.BindConstantBuffer(RootBindings::RootParam::PerView, m_constantBufferManager->GetPerViewGpuAddress());
}

void ForwardOpaquePass::BindGlobalResources(CommandContext& cmd)
{
	m_descriptorHeapManager->SetShaderVisibleHeaps(cmd);

	if (m_samplerLibrary->IsInitialized())
	{
		cmd.BindDescriptorTable(RootBindings::RootParam::SamplerTable, m_samplerLibrary->GetTableGPUHandle());
	}
}

void ForwardOpaquePass::DrawOpaqueMeshes(CommandContext& cmd, const FrameContext& frame)
{
	for (const auto& draw : frame.sceneView.meshDraws)
	{
		const auto* cpuMesh = static_cast<const Mesh*>(draw.meshPtr);
		GPUMesh* gpuMesh = m_gpuMeshCache->GetOrUpload(*cpuMesh);

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
		    m_constantBufferManager->UpdatePerObjectPS(frame.sceneView.materials[draw.materialId].ToPerObjectPSData()));

		const D3D12_GPU_DESCRIPTOR_HANDLE materialTextureTable = frame.sceneView.materials[draw.materialId].textureTableGpuHandle;
		if (materialTextureTable.ptr == 0)
		{
			LOG_WARNING("ForwardOpaquePass::DrawOpaqueMeshes: Material texture table is invalid; draw skipped.");
			continue;
		}

		cmd.BindDescriptorTable(RootBindings::RootParam::TextureSRV, materialTextureTable);

		cmd.DrawIndexedInstanced(gpuMesh->GetIndexCount(), 1, 0, 0, 0);
	}
}
