#include "../PCH.h"
#include "Passes/ForwardOpaquePass.h"

#include "GPU/CommandContext.h"
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

#include <cassert>

namespace
{
	std::shared_ptr<spdlog::logger> g_forwardOpaquePassLogger = Engine::Logging::GetOrCreateLogger("Renderer.ForwardOpaquePass");
}

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

ShaderPackageDefinition ForwardOpaquePass::DescribePrimaryViewShaderPackage() noexcept
{
	return ShaderPackageDefinition{
	    .PackageId = PassName,
	    .VariantId = "Default",
	    .BindingLayoutId = PassName,
	    .ExpectedStages = ShaderStageMask::Vertex | ShaderStageMask::Pixel};
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
	parameters->PerFrame = renderPassContext.HardwareInterface.GetPerFrameConstantData();
	parameters->PerView = viewContext.perViewData;
	const bool valid = parameters.Sync();
	assert(valid);
}

void ForwardOpaquePass::ConfigurePipeline(CommandContext& cmd, const RenderViewContext& viewContext)
{
	cmd.SetViewport(viewContext.viewport);
	cmd.SetScissorRect(viewContext.scissorRect);
	cmd.SetPrimitiveTopology(RhiPrimitiveTopology::TriangleList);
}

void ForwardOpaquePass::BindPassResources(
    const FrameGraph& frameGraph,
    CommandContext& cmd,
    const ParameterInstance& parameters,
    const ForwardOpaquePassRuntime& runtime,
    const RenderPassContext& renderPassContext,
    RhiGpuVirtualAddress perViewGpuAddress)
{
	RenderHardwareInterface& renderHardwareInterface = renderPassContext.HardwareInterface;
	PassBindingOverrides overrides;
	overrides.SetConstantBufferView("PerFrame", renderHardwareInterface.GetPerFrameConstantGpuAddress());
	overrides.SetConstantBufferView("PerView", perViewGpuAddress);
	assert(renderPassContext.SamplerTableHandle);
	overrides.SetDescriptorTable("SamplerTable", renderPassContext.SamplerTableHandle);
	const bool bound = PassUtilities::BindRasterPassWithRuntime(
	    frameGraph,
	    cmd,
	    &renderHardwareInterface,
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
			SPDLOG_LOGGER_WARN(g_forwardOpaquePassLogger, "ForwardOpaquePass::DrawOpaqueMeshes: Material texture table is invalid; draw skipped.");
			continue;
		}

		PassBindingOverrides overrides;
		overrides.SetConstantBufferView("PerObjectVS", renderHardwareInterface.AllocatePerObjectVertexConstants(perObjectVS));
		overrides.SetConstantBufferView("PerObjectPS", renderHardwareInterface.AllocatePerObjectPixelConstants(perObjectPS));
		overrides.SetDescriptorTable("MaterialTextures", materialTextureTable);
		const bool bound = PassUtilities::BindRasterPassOverridesWithRuntime(
		    frameGraph,
		    cmd,
		    &renderHardwareInterface,
		    runtime,
		    RenderPassPipelineTraits<ForwardOpaquePass>::DrawBindingNames,
		    overrides,
		    PassName);
		assert(bound);

		cmd.DrawIndexedInstanced(gpuMesh->GetIndexCount(), 1, 0, 0, 0);
	}
}
