#include "PCH.h"
#include "Passes/Deferred/GBufferPass.h"

#include "Config/DepthConvention.h"
#include "Commands/RenderCommandContext.h"
#include "Diagnostics/PassExecutionDiagnostics.h"
#include "Core/Public/Diagnostics/Logger.h"
#include "Frame/Core/FrameContext.h"
#include "Frame/Core/RenderViewData.h"
#include "Frame/Deferred/GBufferFormats.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "Passes/Deferred/GBufferMeshBatchDrawer.h"
#include "Passes/Core/PassUtilities.h"
#include "Passes/Core/RasterPassUtilities.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "SceneData/RenderSceneData.h"
#include "SceneData/MaterialData.h"
#include "Renderer/Public/SceneData/MeshDraw.h"
#include "Meshes/GPUMesh.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

#include "RHI/Public/Bindings/RenderBindingSet.h"
#include "RHI/Public/Samplers/RhiSamplerDesc.h"
#include "ShaderData/RenderConstantBufferData.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "Pipeline/PassBinder.h"

#include <array>
#include <cassert>

static const auto g_gbufferPassLogger = Logging::GetOrCreateLogger("Renderer.GBufferPass");

GBufferPass::GBufferPass(const RasterPassPipelineRuntime& runtime) noexcept : m_runtime(runtime) {}

const GBufferPass::ParameterMetadata& GBufferPass::GetParameterMetadata() noexcept
{
	return RasterPassUtilities::BuildParameterMetadata<GBufferPass>();
}

const GBufferPass::DrawParameterMetadata& GBufferPass::GetDrawParameterMetadata() noexcept
{
	static const DrawParameterMetadata metadata = []
	{
		return ShaderParameterStructBuilder<DrawParameters>::BuildMetadata("GBuffer.Draw");
	}();

	return metadata;
}

const RenderPassDefinition& GBufferPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition{
	    .PassName = PassName,
	    .ShaderPackage =
	        ShaderPackageDefinition{
	            .PackageId = RendererShaderPackages::GBuffer.data(),
	            .ExpectedStages = ShaderStageMask::Vertex | ShaderStageMask::Pixel},
	    .PipelineKind = RenderPassDefinitionPipelineKind::Graphics,
	    .AllowInputAssemblerInputLayout = true,
	    .BindingLayoutDebugName = L"GBuffer_BindingLayout",
	    .PipelineStateDebugName = L"GBuffer_PipelineState",
	    .Graphics = RenderPassGraphicsPipelineDefinition{
	        .VertexLayout = RhiVertexLayoutKind::StaticMesh,
	        .DepthTest =
	            RhiDepthTestDesc{
	                .DepthEnable = true,
	                .DepthWriteEnable = true,
	                .DepthFunc = DepthConvention::GetDepthComparisonLessEqualFunc()},
	        .RenderTargetFormats =
	            {GBufferFormats::BaseColor,
	             GBufferFormats::Normal,
	             GBufferFormats::Material,
	             GBufferFormats::Emissive,
	             GBufferFormats::Subsurface,
	             GBufferFormats::MotionVector},
	        .RenderTargetCount = 6,
	        .DepthStencilFormat = GBufferFormats::RasterizedDeviceZ}};
	return definition;
}

void GBufferPass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
	SetParameters(parameters, context.Frame.mainView, context.RuntimeServices);
	ConfigurePipeline(context.Commands, context.Frame.mainView);
	PrepareTargets(context, parameters.GetFields());
	BindPassResources(context.Resources, context.Commands, parameters, context.RuntimeServices);
	DrawOpaqueMeshes(context.Resources, context.Commands, context.Frame, parameters.GetFields(), context.RuntimeServices);
}

void GBufferPass::SetParameters(
    ParameterInstance& parameters,
    const RenderViewData& viewData,
    const PassRuntimeServices& passRuntimeServices) const
{
	parameters->PerFrame = passRuntimeServices.PerFrame;
	parameters->PerView = viewData.perViewData;
	parameters->PerTemporal = viewData.perTemporalData;
	parameters->SamplerAniso16xWrap = RhiSamplerDesc{.MaxAnisotropy = RhiSamplerAnisotropy::X16};
	const bool valid = parameters.Sync();
	assert(valid);
}

void GBufferPass::PrepareTargets(PassExecutionContext& context, const GBufferPass::Parameters& parameters) const
{
	const std::array<FrameGraphTextureHandle, 6> renderTargets = {
	    parameters.BaseColor[0],
	    parameters.Normal[0],
	    parameters.Material[0],
	    parameters.Emissive[0],
	    parameters.Subsurface[0],
	    parameters.MotionVector[0]};
	context.Resources.BindRenderTargets(context.Commands, renderTargets, parameters.DeviceZ[0]);
	for (FrameGraphTextureHandle renderTarget : renderTargets)
	{
		context.Resources.ClearRenderTarget(context.Commands, renderTarget);
	}
	context.Resources.ClearDepthStencil(context.Commands, parameters.DeviceZ[0]);
}

void GBufferPass::ConfigurePipeline(RenderCommandContext& cmd, const RenderViewData& viewData) const
{
	cmd.SetViewport(viewData.viewport);
	cmd.SetScissorRect(viewData.scissorRect);
	cmd.SetPrimitiveTopology(RhiPrimitiveTopology::TriangleList);
}

void GBufferPass::BindPassResources(
    const FrameGraphResourceCommands& resources,
    RenderCommandContext& cmd,
    const ParameterInstance& parameters,
    const PassRuntimeServices& passRuntimeServices) const
{
	RenderHardwareInterface& renderHardwareInterface = passRuntimeServices.HardwareInterface;
	const bool bound = PassUtilities::BindAvailableRasterPassWithRuntime(
	    resources,
	    cmd,
	    &renderHardwareInterface,
	    m_runtime,
	    parameters.GetPassParameterSet(),
	    nullptr,
	    PassName,
	    true,
	    passRuntimeServices.PerFrame.ViewModeIndex);
	assert(bound);
}

void GBufferPass::DrawOpaqueMeshes(
    const FrameGraphResourceCommands& resources,
    RenderCommandContext& cmd,
    const FrameContext& frame,
    const Parameters& parameters,
    const PassRuntimeServices& passRuntimeServices) const
{
	GBufferMeshBatchDrawer::DrawOpaqueMeshes(resources, cmd, frame, parameters, passRuntimeServices, m_runtime, GetDrawParameterMetadata());
}
