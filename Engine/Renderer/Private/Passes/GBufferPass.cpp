#include "../PCH.h"
#include "Passes/GBufferPass.h"

#include "GPU/CommandContext.h"
#include "GPU/PassExecutionDiagnostics.h"
#include "Core/Public/Diagnostics/Logger.h"
#include "Core/Public/Diagnostics/Trace.h"
#include "Frame/RenderViewContext.h"
#include "FrameGraph/FrameGraph.h"
#include "Renderer/Public/FrameGraph/RenderGraphPassContext.h"
#include "FrameGraph/RenderPassContext.h"
#include "Passes/PassUtilities.h"
#include "Passes/ShaderPass.h"
#include "SceneData/RenderSceneData.h"
#include "SceneData/MaterialData.h"
#include "Renderer/Public/SceneData/MeshDraw.h"
#include "GPU/GPUMesh.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"

#include "RHI/Public/Resources/RenderConstantBufferData.h"
#include "RHI/Public/Interop/RenderHardwareInterface.h"
#include "Pipeline/RenderPassPipelineTraits.h"
#include "Pipeline/PassBinder.h"

#include <array>
#include <cassert>

const GBufferPass::ParameterMetadata& GBufferPass::GetParameterMetadata() noexcept
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

const GBufferPass::DrawParameterMetadata& GBufferPass::GetDrawParameterMetadata() noexcept
{
	static const DrawParameterMetadata metadata = []
	{
		return ShaderParameterStructBuilder<DrawParameters>::BuildMetadata("GBuffer.Draw");
	}();

	return metadata;
}

ShaderPackageDefinition GBufferPass::DescribeGBufferShaderPackage() noexcept
{
	return ShaderPackageDefinition{
	    .PackageId = PassName,
	    .VariantId = "Default",
	    .BindingLayoutId = PassName,
	    .ExpectedStages = ShaderStageMask::Vertex | ShaderStageMask::Pixel};
}

void GBufferPass::Execute(RenderGraphPassContext& context, ParameterInstance& parameters)
{
	SPARKLE_GPU_PASS_SCOPE(context.Diagnostics, "Renderer.GBuffer.Execute");

	const GBufferPassRuntime& runtime = context.Runtime.GetPassRuntime<GBufferPass>();
	SetParameters(parameters, context.Frame.mainView);
	PrepareTargets(context, parameters.GetFields());
	ConfigurePipeline(context.Commands, context.Frame.mainView);
	BindPassResources(context.Graph, context.Commands, parameters, runtime, context.Runtime);
	DrawOpaqueMeshes(context.Graph, context.Commands, context.Frame.sceneData, runtime, context.Runtime);
}

void GBufferPass::DeclareResources(FrameGraph& frameGraph, const GBufferTargets& targets, ParameterInstance& parameters)
{
	parameters->BaseColor = frameGraph.CreateRenderTarget(targets.BaseColor);
	parameters->Normal = frameGraph.CreateRenderTarget(targets.Normal);
	parameters->Material = frameGraph.CreateRenderTarget(targets.Material);
	parameters->Emissive = frameGraph.CreateRenderTarget(targets.Emissive);
	parameters->Subsurface = frameGraph.CreateRenderTarget(targets.Subsurface);
	parameters->DeviceZ = frameGraph.CreateRenderTarget(targets.DeviceZ);
	parameters->MainDepth = frameGraph.CreateDepthTarget(targets.MainDepth);
	parameters->SamplerAniso16xWrap = RhiSamplerDesc{.MaxAnisotropy = RhiSamplerAnisotropy::X16};
}

void GBufferPass::SetParameters(ParameterInstance& parameters, const RenderViewContext& viewContext)
{
	parameters->PerView = viewContext.perViewData;
	const bool valid = parameters.Sync();
	assert(valid);
}

void GBufferPass::PrepareTargets(RenderGraphPassContext& context, const GBufferPass::Parameters& parameters)
{
	const std::array<TextureHandle, 6> renderTargets = {
	    parameters.BaseColor[0],
	    parameters.Normal[0],
	    parameters.Material[0],
	    parameters.Emissive[0],
		    parameters.Subsurface[0],
	    parameters.DeviceZ[0]};
	context.Graph.BindRenderTargets(context.Commands, renderTargets, parameters.MainDepth[0]);
	for (TextureHandle renderTarget : renderTargets)
	{
		context.Graph.ClearRenderTarget(context.Commands, renderTarget);
	}
	context.Graph.ClearDepthStencil(context.Commands, parameters.MainDepth[0]);
}

void GBufferPass::ConfigurePipeline(CommandContext& cmd, const RenderViewContext& viewContext)
{
	cmd.SetViewport(viewContext.viewport);
	cmd.SetScissorRect(viewContext.scissorRect);
	cmd.SetPrimitiveTopology(RhiPrimitiveTopology::TriangleList);
}

void GBufferPass::BindPassResources(
    const FrameGraph& frameGraph,
    CommandContext& cmd,
    const ParameterInstance& parameters,
    const GBufferPassRuntime& runtime,
    const RenderPassContext& renderPassContext)
{
	RenderHardwareInterface& renderHardwareInterface = renderPassContext.HardwareInterface;
	const bool bound = PassUtilities::BindAvailableRasterPassWithRuntime(
	    frameGraph,
	    cmd,
	    &renderHardwareInterface,
	    runtime,
	    parameters.GetPassParameterSet(),
	    nullptr,
	    PassName);
	assert(bound);
}

void GBufferPass::DrawOpaqueMeshes(
    const FrameGraph& frameGraph,
    CommandContext& cmd,
    const RenderSceneData& sceneData,
    const GBufferPassRuntime& runtime,
    const RenderPassContext& renderPassContext)
{
	RenderHardwareInterface& renderHardwareInterface = renderPassContext.HardwareInterface;

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

		const RhiDescriptorTableHandle materialTextureTable = sceneData.materials[draw.materialSlot].textureTableHandle;
		if (!materialTextureTable)
		{
			SPDLOG_LOGGER_WARN(
			    Logging::GetOrCreateLogger("Renderer.GBufferPass"),
			    "GBufferPass::DrawOpaqueMeshes: Material texture table is invalid; draw skipped.");
			continue;
		}

		DrawParameterInstance drawParameters(GetDrawParameterMetadata());
		drawParameters->PerObjectVS = perObjectVS;
		drawParameters->PerObjectPS = perObjectPS;
		drawParameters->TextureBaseColor = RhiDescriptorTableBinding{materialTextureTable, MaterialTextureSlots::BaseColor};
		drawParameters->TextureNormal = RhiDescriptorTableBinding{materialTextureTable, MaterialTextureSlots::Normal};
		drawParameters->TextureMetallicRoughness = RhiDescriptorTableBinding{materialTextureTable, MaterialTextureSlots::MetallicRoughness};
		drawParameters->TextureOcclusion = RhiDescriptorTableBinding{materialTextureTable, MaterialTextureSlots::Occlusion};
		drawParameters->TextureEmissive = RhiDescriptorTableBinding{materialTextureTable, MaterialTextureSlots::Emissive};
		const bool bound = PassUtilities::BindAvailableRasterPassWithRuntime(
		    frameGraph,
		    cmd,
		    &renderHardwareInterface,
		    runtime,
		    drawParameters.GetPassParameterSet(),
		    nullptr,
		    PassName);
		assert(bound);

		cmd.DrawIndexedInstanced(gpuMesh->GetIndexCount(), 1, 0, 0, 0);
	}
}
