#include "../PCH.h"
#include "Passes/GBufferPass.h"

#include "Commands/RenderCommandContext.h"
#include "Diagnostics/PassExecutionDiagnostics.h"
#include "Core/Public/Diagnostics/Logger.h"
#include "Core/Public/Diagnostics/Trace.h"
#include "Frame/FrameContext.h"
#include "Frame/RenderViewData.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "Passes/GBufferMeshBatchDrawer.h"
#include "Passes/PassUtilities.h"
#include "Passes/ShaderPass.h"
#include "SceneData/RenderSceneData.h"
#include "SceneData/MaterialData.h"
#include "Renderer/Public/SceneData/MeshDraw.h"
#include "Meshes/GPUMesh.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"

#include "RHI/Public/Bindings/RenderBindingSet.h"
#include "RHI/Public/Resources/RenderConstantBufferData.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "Pipeline/PassBinder.h"

#include <array>
#include <cassert>

static const auto g_gbufferPassLogger = Logging::GetOrCreateLogger("Renderer.GBufferPass");

GBufferPass::GBufferPass(const RasterPassPipelineRuntime& runtime) noexcept : m_runtime(runtime) {}

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
	    .BindingLayoutId = PassName,
	    .ExpectedStages = ShaderStageMask::Vertex | ShaderStageMask::Pixel};
}

void GBufferPass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
	SPARKLE_GPU_PASS_SCOPE(context.Diagnostics, "Renderer.GBuffer.Execute");

	SetParameters(parameters, context.Frame.mainView, context.RuntimeServices);
	PrepareTargets(context, parameters.GetFields());
	ConfigurePipeline(context.Commands, context.Frame.mainView);
	BindPassResources(context.Resources, context.Commands, parameters, context.RuntimeServices);
	DrawOpaqueMeshes(context.Resources, context.Commands, context.Frame, context.RuntimeServices);
}

void GBufferPass::DeclareResources(FrameGraphBuilder& builder, const GBufferRenderTargets& targets, ParameterInstance& parameters)
{
	parameters->BaseColor = builder.CreateRenderTarget(targets.BaseColor);
	parameters->Normal = builder.CreateRenderTarget(targets.Normal);
	parameters->Material = builder.CreateRenderTarget(targets.Material);
	parameters->Emissive = builder.CreateRenderTarget(targets.Emissive);
	parameters->Subsurface = builder.CreateRenderTarget(targets.Subsurface);
	parameters->DeviceZ = builder.CreateRenderTarget(targets.DeviceZ);
	parameters->MotionVector = builder.CreateRenderTarget(targets.MotionVector);
	parameters->MainDepth = builder.CreateDepthTarget(targets.MainDepth);
	parameters->SamplerAniso16xWrap = RhiSamplerDesc{.MaxAnisotropy = RhiSamplerAnisotropy::X16};
}

void GBufferPass::SetParameters(
    ParameterInstance& parameters,
    const RenderViewData& viewData,
    const PassRuntimeServices& passRuntimeServices) const
{
	parameters->PerFrame = passRuntimeServices.HardwareInterface.GetPerFrameConstantData();
	parameters->PerView = viewData.perViewData;
	parameters->PerTemporal = viewData.perTemporalData;
	const bool valid = parameters.Sync();
	assert(valid);
}

void GBufferPass::PrepareTargets(PassExecutionContext& context, const GBufferPass::Parameters& parameters) const
{
	const std::array<FrameGraphTextureHandle, 7> renderTargets = {
	    parameters.BaseColor[0],
	    parameters.Normal[0],
	    parameters.Material[0],
	    parameters.Emissive[0],
	    parameters.Subsurface[0],
	    parameters.DeviceZ[0],
	    parameters.MotionVector[0]};
	context.Resources.BindRenderTargets(context.Commands, renderTargets, parameters.MainDepth[0]);
	for (FrameGraphTextureHandle renderTarget : renderTargets)
	{
		context.Resources.ClearRenderTarget(context.Commands, renderTarget);
	}
	context.Resources.ClearDepthStencil(context.Commands, parameters.MainDepth[0]);
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
	    PassName);
	assert(bound);
}

void GBufferPass::DrawOpaqueMeshes(
	const FrameGraphResourceCommands& resources,
	RenderCommandContext& cmd,
	const FrameContext& frame,
	const PassRuntimeServices& passRuntimeServices) const
{
	GBufferMeshBatchDrawer::DrawOpaqueMeshes(resources, cmd, frame, passRuntimeServices, m_runtime, GetDrawParameterMetadata());
}
