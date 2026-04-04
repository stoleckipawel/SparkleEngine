#include "../PCH.h"
#include "Renderer/Public/Passes/ForwardOpaquePass.h"

#include "Renderer/Public/GPU/CommandContext.h"
#include "Renderer/Public/Frame/RenderViewContext.h"
#include "Renderer/Public/FrameGraph/FrameGraph.h"
#include "Renderer/Public/FrameGraph/RenderGraphPassContext.h"
#include "Renderer/Public/FrameGraph/RenderPassContext.h"
#include "Renderer/Public/Passes/PassUtilities.h"
#include "Renderer/Public/Passes/ShaderPass.h"
#include "Renderer/Public/SceneData/RenderSceneData.h"
#include "Renderer/Public/SceneData/MaterialData.h"
#include "Renderer/Public/SceneData/MeshDraw.h"
#include "Renderer/Public/GPU/GPUMesh.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"

#include "D3D12/Pipeline/D3D12BindingLayout.h"
#include "D3D12/Pipeline/D3D12PipelineState.h"
#include "D3D12/Pipeline/D3D12PassBinder.h"
#include "D3D12/Resources/D3D12ConstantBufferManager.h"
#include "D3D12/Resources/D3D12ConstantBufferData.h"
#include "D3D12/Descriptors/D3D12DescriptorHeapManager.h"
#include "Pipeline/RenderPassPipelineTraits.h"
#include "D3D12/Samplers/D3D12SamplerLibrary.h"

#include "Core/Public/Diagnostics/Log.h"
#include <cassert>

const ForwardOpaquePass::ParameterMetadata& ForwardOpaquePass::GetParameterMetadata() noexcept
{
	static const ParameterMetadata metadata = []
	{
		const ParameterMetadata localMetadata = ShaderParameterStructBuilder<Parameters>::BuildMetadata(PassName);
		const bool valid = ValidateShaderPassLayout(localMetadata.GetLayout(), ShaderPassKind::Raster, PassName);
		assert(valid);
		return localMetadata;
	}();

	return metadata;
}

ShaderSourceDefinition ForwardOpaquePass::DescribePrimaryViewVertexShader() noexcept
{
	return ShaderSourceDefinition::FromAsset("Passes/Forward/ForwardLitVS.hlsl", "main", ShaderStage::Vertex);
}

ShaderSourceDefinition ForwardOpaquePass::DescribePrimaryViewPixelShader() noexcept
{
	return ShaderSourceDefinition::FromAsset("Passes/Forward/ForwardLitPS.hlsl", "main", ShaderStage::Pixel);
}

void ForwardOpaquePass::Execute(RenderGraphPassContext& context, ParameterInstance& parameters)
{
	const ForwardOpaquePassRuntime& runtime = context.Runtime.GetPassRuntime<ForwardOpaquePass>();
	PreparePassParameters(parameters, context.Frame.mainView, context.Runtime);
	PrepareTargets(context, parameters.GetFields());
	ConfigurePipeline(context.Commands, context.Frame.mainView);
	BindPassResources(context.Graph, context.Commands, parameters, runtime, context.Runtime, context.Frame.mainView.perViewGpuAddress);
	DrawOpaqueMeshes(context.Graph, context.Commands, context.Frame.sceneData, runtime, context.Runtime);
}

void ForwardOpaquePass::PrepareTargets(RenderGraphPassContext& context, const ForwardOpaquePass::Parameters& parameters)
{
	context.Graph.BindRenderTarget(context.Commands, parameters.BackBuffer[0], parameters.MainDepth[0]);
	context.Graph.ClearRenderTarget(context.Commands, parameters.BackBuffer[0]);
	context.Graph.ClearDepthStencil(context.Commands, parameters.MainDepth[0]);
}

void ForwardOpaquePass::PreparePassParameters(
    ParameterInstance& parameters,
    const RenderViewContext& viewContext,
    const RenderPassContext& renderPassContext)
{
	parameters->PerFrame = renderPassContext.ConstantBufferManager.GetPerFrameData();
	parameters->PerView = viewContext.perViewData;
	const bool valid = parameters.Sync();
	assert(valid);
}

void ForwardOpaquePass::ConfigurePipeline(CommandContext& cmd, const RenderViewContext& viewContext)
{
	const D3D12_VIEWPORT viewport = viewContext.viewport;
	cmd.SetViewport(viewport.TopLeftX, viewport.TopLeftY, viewport.Width, viewport.Height, viewport.MinDepth, viewport.MaxDepth);

	const D3D12_RECT scissor = viewContext.scissorRect;
	cmd.SetScissorRect(scissor.left, scissor.top, scissor.right, scissor.bottom);
	cmd.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void ForwardOpaquePass::BindPassResources(
    const FrameGraph& frameGraph,
    CommandContext& cmd,
    const ParameterInstance& parameters,
    const ForwardOpaquePassRuntime& runtime,
    const RenderPassContext& renderPassContext,
    D3D12_GPU_VIRTUAL_ADDRESS perViewGpuAddress)
{
	D3D12DescriptorHeapManager& descriptorHeapManager = renderPassContext.DescriptorHeapManager;
	D3D12ConstantBufferManager& constantBufferManager = renderPassContext.ConstantBufferManager;
	D3D12SamplerLibrary& samplerLibrary = renderPassContext.SamplerLibrary;
	D3D12PassBindingOverrides overrides;
	overrides.SetConstantBufferView("PerFrame", constantBufferManager.GetPerFrameGpuAddress());
	overrides.SetConstantBufferView("PerView", perViewGpuAddress);
	assert(samplerLibrary.IsInitialized());
	overrides.SetDescriptorTable("SamplerTable", samplerLibrary.GetTableGPUHandle());
	const bool bound = PassUtilities::BindRasterPassWithRuntime(
	    frameGraph,
	    cmd,
	    &descriptorHeapManager,
	    runtime,
	    parameters.GetPassParameterSet(),
	    RenderPassPipelineTraits<ForwardOpaquePass>::StableBindingNames.data(),
	    static_cast<std::uint32_t>(RenderPassPipelineTraits<ForwardOpaquePass>::StableBindingNames.size()),
	    &overrides,
	    PassName);
	assert(bound);
}

void ForwardOpaquePass::DrawOpaqueMeshes(
    const FrameGraph& frameGraph,
    CommandContext& cmd,
    const RenderSceneData& sceneData,
    const ForwardOpaquePassRuntime& runtime,
    const RenderPassContext& renderPassContext)
{
	D3D12DescriptorHeapManager& descriptorHeapManager = renderPassContext.DescriptorHeapManager;
	D3D12ConstantBufferManager& constantBufferManager = renderPassContext.ConstantBufferManager;

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
		const PerObjectPSConstantBufferData perObjectPS = sceneData.materials[draw.materialSlot].ToPerObjectPSData();

		const D3D12_GPU_DESCRIPTOR_HANDLE materialTextureTable = sceneData.materials[draw.materialSlot].textureTableGpuHandle;
		if (materialTextureTable.ptr == 0)
		{
			LOG_WARNING("ForwardOpaquePass::DrawOpaqueMeshes: Material texture table is invalid; draw skipped.");
			continue;
		}

		D3D12PassBindingOverrides overrides;
		overrides.SetConstantBufferView("PerObjectVS", constantBufferManager.UpdatePerObjectVS(perObjectVS));
		overrides.SetConstantBufferView("PerObjectPS", constantBufferManager.UpdatePerObjectPS(perObjectPS));
		overrides.SetDescriptorTable("MaterialTextures", materialTextureTable);
		const bool bound = PassUtilities::BindRasterPassOverridesWithRuntime(
		    frameGraph,
		    cmd,
		    &descriptorHeapManager,
		    runtime,
		    RenderPassPipelineTraits<ForwardOpaquePass>::DrawBindingNames,
		    overrides,
		    PassName);
		assert(bound);

		cmd.DrawIndexedInstanced(gpuMesh->GetIndexCount(), 1, 0, 0, 0);
	}
}
