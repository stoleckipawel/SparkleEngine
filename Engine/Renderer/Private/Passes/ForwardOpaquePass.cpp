#include "PCH.h"
#include "Renderer/Public/Passes/ForwardOpaquePass.h"

#include "Renderer/Public/RenderContext.h"
#include "Renderer/Public/SceneData/SceneView.h"
#include "Renderer/Public/SceneData/MeshDraw.h"
#include "Renderer/Public/GPU/GPUMesh.h"
#include "Renderer/Public/GPU/GPUMeshCache.h"
#include "Renderer/Public/TextureManager.h"
#include "Renderer/Public/FrameGraph/PassBuilder.h"

#include "D3D12RootSignature.h"
#include "D3D12PipelineState.h"
#include "D3D12ConstantBufferManager.h"
#include "D3D12ConstantBufferData.h"
#include "D3D12RootBindings.h"
#include "D3D12DescriptorHeapManager.h"
#include "D3D12Texture.h"
#include "Samplers/D3D12SamplerLibrary.h"
#include "D3D12SwapChain.h"
#include "D3D12DepthStencil.h"
#include "Scene/Mesh.h"

#include "Core/Public/Diagnostics/Log.h"

ForwardOpaquePass::ForwardOpaquePass(
    std::string_view name,
    D3D12RootSignature& rootSignature,
    D3D12PipelineState& pipelineState,
    D3D12ConstantBufferManager& constantBufferManager,
    D3D12DescriptorHeapManager& descriptorHeapManager,
    TextureManager& textureManager,
    D3D12SamplerLibrary& samplerLibrary,
    GPUMeshCache& gpuMeshCache,
    D3D12SwapChain& swapChain,
    D3D12DepthStencil& depthStencil) noexcept :
    RenderPass(name),
    m_rootSignature(&rootSignature),
    m_pipelineState(&pipelineState),
    m_constantBufferManager(&constantBufferManager),
    m_descriptorHeapManager(&descriptorHeapManager),
    m_textureManager(&textureManager),
    m_samplerLibrary(&samplerLibrary),
    m_gpuMeshCache(&gpuMeshCache),
    m_swapChain(&swapChain),
    m_depthStencil(&depthStencil)
{
	LOG_INFO("ForwardOpaquePass: Created");
}

void ForwardOpaquePass::Setup(PassBuilder& builder, const SceneView& sceneView)
{
	m_sceneView = &sceneView;
	m_backBuffer = builder.UseBackBuffer();
	m_depthBuffer = builder.UseDepthBuffer();
}

void ForwardOpaquePass::Execute(RenderContext& context)
{
	PrepareTargets(context);
	ConfigurePipeline(context);
	BindFrameResources(context);
	BindGlobalResources(context);
	DrawOpaqueMeshes(context);
}

void ForwardOpaquePass::PrepareTargets(RenderContext& context)
{
	m_swapChain->SetRenderTargetState();
	m_depthStencil->SetWriteState();

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_swapChain->GetCPUHandle();
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_depthStencil->GetCPUHandle();
	context.SetRenderTarget(rtvHandle, &dsvHandle);

	m_swapChain->Clear();
	m_depthStencil->Clear();
}

void ForwardOpaquePass::ConfigurePipeline(RenderContext& context)
{
	context.SetRootSignature(m_rootSignature->GetRaw());

	const D3D12_VIEWPORT viewport = m_swapChain->GetDefaultViewport();
	context.SetViewport(viewport.TopLeftX, viewport.TopLeftY, viewport.Width, viewport.Height, viewport.MinDepth, viewport.MaxDepth);

	const D3D12_RECT scissor = m_swapChain->GetDefaultScissorRect();
	context.SetScissorRect(scissor.left, scissor.top, scissor.right, scissor.bottom);

	context.SetPipelineState(m_pipelineState->Get().Get());
	context.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void ForwardOpaquePass::BindFrameResources(RenderContext& context)
{
	context.BindConstantBuffer(RootBindings::RootParam::PerFrame, m_constantBufferManager->GetPerFrameGpuAddress());

	context.BindConstantBuffer(RootBindings::RootParam::PerView, m_constantBufferManager->GetPerViewGpuAddress());
}

void ForwardOpaquePass::BindGlobalResources(RenderContext& context)
{
	m_descriptorHeapManager->SetShaderVisibleHeaps();

	if (m_samplerLibrary->IsInitialized())
	{
		context.BindDescriptorTable(RootBindings::RootParam::SamplerTable, m_samplerLibrary->GetTableGPUHandle());
	}
}

void ForwardOpaquePass::DrawOpaqueMeshes(RenderContext& context)
{
	for (const auto& draw : m_sceneView->meshDraws)
	{
		const auto* cpuMesh = static_cast<const Mesh*>(draw.meshPtr);
		GPUMesh* gpuMesh = m_gpuMeshCache->GetOrUpload(*cpuMesh);

		if (!gpuMesh || !gpuMesh->IsValid())
		{
			continue;
		}

		context.BindVertexBuffer(gpuMesh->GetVertexBufferView());
		context.BindIndexBuffer(gpuMesh->GetIndexBufferView());

		PerObjectVSConstantBufferData perObjectVS{};
		perObjectVS.WorldMTX = draw.worldMatrix;
		perObjectVS.WorldInvTransposeMTX = draw.worldInvTranspose;

		context.BindConstantBuffer(RootBindings::RootParam::PerObjectVS, m_constantBufferManager->UpdatePerObjectVS(perObjectVS));

		context.BindConstantBuffer(
		    RootBindings::RootParam::PerObjectPS,
		    m_constantBufferManager->UpdatePerObjectPS(m_sceneView->materials[draw.materialId].ToPerObjectPSData()));

		const D3D12_GPU_DESCRIPTOR_HANDLE materialTextureTable = m_sceneView->materials[draw.materialId].textureTableGpuHandle;
		if (materialTextureTable.ptr == 0)
		{
			LOG_WARNING("ForwardOpaquePass::DrawOpaqueMeshes: Material texture table is invalid; draw skipped.");
			continue;
		}

		context.BindDescriptorTable(RootBindings::RootParam::TextureSRV, materialTextureTable);

		context.DrawIndexedInstanced(gpuMesh->GetIndexCount(), 1, 0, 0, 0);
	}
}
