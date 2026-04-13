#include "../PCH.h"

#include "Passes/ShadowOpaquePass.h"

#include "GPU/CommandContext.h"
#include "Frame/RenderViewContext.h"
#include "FrameGraph/FrameGraph.h"
#include "Renderer/Public/FrameGraph/RenderGraphPassContext.h"
#include "FrameGraph/RenderPassContext.h"
#include "Passes/PassUtilities.h"
#include "Passes/ShaderPass.h"
#include "GPU/GPUMesh.h"
#include "Renderer/Public/SceneData/MeshDraw.h"
#include "SceneData/RenderSceneData.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"

#include "D3D12/Pipeline/D3D12BindingLayout.h"
#include "D3D12/Resources/D3D12ConstantBufferData.h"
#include "D3D12/Resources/D3D12ConstantBufferManager.h"
#include "D3D12/Pipeline/D3D12PipelineState.h"
#include "Pipeline/RenderPassPipelineTraits.h"
#include "Pipeline/D3D12PassBinder.h"

#include <cassert>

const ShadowOpaquePass::ParameterMetadata& ShadowOpaquePass::GetParameterMetadata() noexcept
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

ShaderSourceDefinition ShadowOpaquePass::DescribeShadowViewVertexShader() noexcept
{
	return ShaderSourceDefinition::FromAsset("Passes/Shadow/ShadowDepthVS.hlsl", "main", ShaderStage::Vertex);
}

ShaderSourceDefinition ShadowOpaquePass::DescribeShadowViewPixelShader() noexcept
{
	return ShaderSourceDefinition::FromAsset("Passes/Shadow/ShadowDepthPS.hlsl", "main", ShaderStage::Pixel);
}

void ShadowOpaquePass::Execute(RenderGraphPassContext& context, ParameterInstance& parameters, std::size_t lightIndex)
{
	if (lightIndex >= context.Frame.shadowViewCount)
	{
		return;
	}

	const ShadowOpaquePassRuntime& runtime = context.Runtime.GetPassRuntime<ShadowOpaquePass>();
	const RenderViewContext& viewContext = context.Frame.shadowViews[lightIndex];
	PreparePassParameters(parameters, viewContext, context.Runtime);
	PrepareTargets(context, parameters.GetFields());
	ConfigurePipeline(context.Commands, viewContext);
	BindPassResources(context.Graph, context.Commands, parameters, runtime, context.Runtime, viewContext.perViewGpuAddress);
	DrawMeshes(context.Graph, context.Commands, context.Frame.sceneData, runtime, context.Runtime);
}

void ShadowOpaquePass::PrepareTargets(RenderGraphPassContext& context, const ShadowOpaquePass::Parameters& parameters)
{
	context.Graph.BindRenderTarget(context.Commands, parameters.ShadowColor[0], parameters.ShadowDepth[0]);
	context.Graph.ClearRenderTarget(context.Commands, parameters.ShadowColor[0]);
	context.Graph.ClearDepthStencil(context.Commands, parameters.ShadowDepth[0]);
}

void ShadowOpaquePass::PreparePassParameters(
    ParameterInstance& parameters,
    const RenderViewContext& viewContext,
    const RenderPassContext& renderPassContext)
{
	parameters->PerFrame = renderPassContext.ConstantBufferManager.GetPerFrameData();
	parameters->PerView = viewContext.perViewData;
	const bool valid = parameters.Sync();
	assert(valid);
}

void ShadowOpaquePass::ConfigurePipeline(CommandContext& cmd, const RenderViewContext& viewContext)
{
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
	cmd.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void ShadowOpaquePass::BindPassResources(
    const FrameGraph& frameGraph,
    CommandContext& cmd,
    const ParameterInstance& parameters,
    const ShadowOpaquePassRuntime& runtime,
    const RenderPassContext& renderPassContext,
    D3D12_GPU_VIRTUAL_ADDRESS perViewGpuAddress)
{
	D3D12ConstantBufferManager& constantBufferManager = renderPassContext.ConstantBufferManager;
	D3D12PassBindingOverrides overrides;
	overrides.SetConstantBufferView("PerFrame", constantBufferManager.GetPerFrameGpuAddress());
	overrides.SetConstantBufferView("PerView", perViewGpuAddress);
	const bool bound = PassUtilities::BindRasterPassWithRuntime(
	    frameGraph,
	    cmd,
	    nullptr,
	    runtime,
	    parameters.GetPassParameterSet(),
	    RenderPassPipelineTraits<ShadowOpaquePass>::StableBindingNames.data(),
	    static_cast<std::uint32_t>(RenderPassPipelineTraits<ShadowOpaquePass>::StableBindingNames.size()),
	    &overrides,
	    PassName);
	assert(bound);
}

void ShadowOpaquePass::DrawMeshes(
    const FrameGraph& frameGraph,
    CommandContext& cmd,
    const RenderSceneData& sceneData,
    const ShadowOpaquePassRuntime& runtime,
    const RenderPassContext& renderPassContext)
{
	D3D12ConstantBufferManager& constantBufferManager = renderPassContext.ConstantBufferManager;

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
		D3D12PassBindingOverrides overrides;
		overrides.SetConstantBufferView("PerObjectVS", constantBufferManager.UpdatePerObjectVS(perObjectVS));
		const bool bound = PassUtilities::BindRasterPassOverridesWithRuntime(
		    frameGraph,
		    cmd,
		    nullptr,
		    runtime,
		    RenderPassPipelineTraits<ShadowOpaquePass>::DrawBindingNames,
		    overrides,
		    PassName);
		assert(bound);

		cmd.DrawIndexedInstanced(gpuMesh->GetIndexCount(), 1, 0, 0, 0);
	}
}