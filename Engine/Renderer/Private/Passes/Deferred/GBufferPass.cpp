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
#include "FrameGraph/PassRuntimeContext.h"
#include "Passes/Deferred/GBufferMeshBatchDrawer.h"
#include "Passes/Core/ShaderPassOperations.h"
#include "Passes/Core/RasterPassOperations.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "SceneData/RenderSceneData.h"
#include "Scene/Materials/MaterialData.h"
#include "Renderer/Public/SceneData/MeshDraw.h"
#include "Meshes/GpuMesh.h"
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

void GBufferPassParameters::Describe(ShaderParameterStructBuilder<GBufferPassParameters>& builder)
{
	builder.RenderTarget("BaseColor", &GBufferPassParameters::BaseColor, ShaderStageVisibility::AllGraphics);
	builder.RenderTarget("Normal", &GBufferPassParameters::Normal, ShaderStageVisibility::AllGraphics);
	builder.RenderTarget("Material", &GBufferPassParameters::Material, ShaderStageVisibility::AllGraphics);
	builder.RenderTarget("Emissive", &GBufferPassParameters::Emissive, ShaderStageVisibility::AllGraphics);
	builder.RenderTarget("Subsurface", &GBufferPassParameters::Subsurface, ShaderStageVisibility::AllGraphics);
	builder.RenderTarget("MotionVector", &GBufferPassParameters::MotionVector, ShaderStageVisibility::AllGraphics);
	builder.DepthTarget("DeviceZ", &GBufferPassParameters::DeviceZ, ShaderStageVisibility::AllGraphics);
	builder.Uniform("PerFrame", &GBufferPassParameters::PerFrame, ShaderStageVisibility::Pixel);
	builder.Uniform("PerView", &GBufferPassParameters::PerView, ShaderStageVisibility::Vertex);
	builder.Uniform("PerTemporal", &GBufferPassParameters::PerTemporal, ShaderStageVisibility::Vertex | ShaderStageVisibility::Pixel);
	builder.Sampler("SamplerAniso16xWrap", &GBufferPassParameters::SamplerAniso16xWrap, ShaderStageVisibility::Pixel);
	builder.ReadBuffer("MeshInstances", &GBufferPassParameters::MeshInstances, ShaderStageVisibility::Vertex);
	builder.ReadBuffer("MeshInstanceSlots", &GBufferPassParameters::MeshInstanceSlots, ShaderStageVisibility::Vertex);
	builder.ReadBuffer("JointMatrices", &GBufferPassParameters::JointMatrices, ShaderStageVisibility::Vertex);
	builder.ReadBuffer("PreviousJointMatrices", &GBufferPassParameters::PreviousJointMatrices, ShaderStageVisibility::Vertex);
	builder.ReadBuffer("MorphWeights", &GBufferPassParameters::MorphWeights, ShaderStageVisibility::Vertex);
	builder.ReadBuffer("PreviousMorphWeights", &GBufferPassParameters::PreviousMorphWeights, ShaderStageVisibility::Vertex);
}

void GBufferDrawParameters::Describe(ShaderParameterStructBuilder<GBufferDrawParameters>& builder)
{
	builder.Uniform("MeshInstanceDraw", &GBufferDrawParameters::MeshInstanceDraw, ShaderStageVisibility::Vertex);
	builder.ReadBuffer("MeshInstances", &GBufferDrawParameters::MeshInstances, ShaderStageVisibility::Vertex);
	builder.ReadBuffer("MeshInstanceSlots", &GBufferDrawParameters::MeshInstanceSlots, ShaderStageVisibility::Vertex);
	builder.ReadBuffer("SkinInfluences", &GBufferDrawParameters::SkinInfluences, ShaderStageVisibility::Vertex);
	builder.ReadBuffer("MorphTargetDeltas", &GBufferDrawParameters::MorphTargetDeltas, ShaderStageVisibility::Vertex);
	builder.ReadBuffer("JointMatrices", &GBufferDrawParameters::JointMatrices, ShaderStageVisibility::Vertex);
	builder.ReadBuffer("PreviousJointMatrices", &GBufferDrawParameters::PreviousJointMatrices, ShaderStageVisibility::Vertex);
	builder.ReadBuffer("MorphWeights", &GBufferDrawParameters::MorphWeights, ShaderStageVisibility::Vertex);
	builder.ReadBuffer("PreviousMorphWeights", &GBufferDrawParameters::PreviousMorphWeights, ShaderStageVisibility::Vertex);
	builder.Uniform("PerObjectPS", &GBufferDrawParameters::PerObjectPS, ShaderStageVisibility::Pixel);
	builder.ReadTexture("TextureBaseColor", &GBufferDrawParameters::TextureBaseColor, ShaderStageVisibility::Pixel);
	builder.ReadTexture("TextureNormal", &GBufferDrawParameters::TextureNormal, ShaderStageVisibility::Pixel);
	builder.ReadTexture("TextureRoughness", &GBufferDrawParameters::TextureRoughness, ShaderStageVisibility::Pixel);
	builder.ReadTexture("TextureMetallic", &GBufferDrawParameters::TextureMetallic, ShaderStageVisibility::Pixel);
	builder.ReadTexture("TextureOcclusion", &GBufferDrawParameters::TextureOcclusion, ShaderStageVisibility::Pixel);
	builder.ReadTexture("TextureEmissive", &GBufferDrawParameters::TextureEmissive, ShaderStageVisibility::Pixel);
	builder.ReadTexture("TextureSubsurfaceColor", &GBufferDrawParameters::TextureSubsurfaceColor, ShaderStageVisibility::Pixel);
	builder.ReadTexture("TextureSubsurfaceStrength", &GBufferDrawParameters::TextureSubsurfaceStrength, ShaderStageVisibility::Pixel);
}

GBufferPass::GBufferPass(const RasterPassPipelineRuntime& runtime) noexcept :
    m_runtime(runtime)
{
}

const GBufferPass::ParameterMetadata& GBufferPass::GetParameterMetadata() noexcept
{
	return RasterPassOperations::BuildParameterMetadata<GBufferPass>();
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
	    .PipelineDebugName = L"GBuffer_Pipeline",
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
	SetParameters(parameters, context.Frame.mainView, context.Runtime);
	ConfigurePipeline(context.Commands, context.Frame.mainView);
	PrepareTargets(context, parameters.GetFields());
	BindPassResources(context.Resources, context.Commands, parameters, context.Runtime);
	DrawOpaqueMeshes(context.Resources, context.Commands, context.Frame, parameters.GetFields(), context.Runtime);
}

void GBufferPass::SetParameters(
    ParameterInstance& parameters,
    const RenderViewData& viewData,
    const PassRuntimeContext& passRuntimeContext) const
{
	parameters->PerFrame = passRuntimeContext.PerFrame;
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

void GBufferPass::ConfigurePipeline(RenderCommandContext& commandContext, const RenderViewData& viewData) const
{
	commandContext.SetViewport(viewData.viewport);
	commandContext.SetScissorRect(viewData.scissorRect);
	commandContext.SetPrimitiveTopology(RhiPrimitiveTopology::TriangleList);
}

void GBufferPass::BindPassResources(
    const FrameGraphResourceCommands& resources,
    RenderCommandContext& commandContext,
    const ParameterInstance& parameters,
    const PassRuntimeContext& passRuntimeContext) const
{
	RenderHardwareInterface& renderHardwareInterface = passRuntimeContext.HardwareInterface;
	const bool bound = ShaderPassOperations::BindAvailableRasterPassWithRuntime(
	    resources,
	    commandContext,
	    &renderHardwareInterface,
	    m_runtime,
	    parameters.GetPassParameterSet(),
	    nullptr,
	    PassName,
	    true,
	    passRuntimeContext.PerFrame.ViewModeIndex);
	assert(bound);
}

void GBufferPass::DrawOpaqueMeshes(
    const FrameGraphResourceCommands& resources,
    RenderCommandContext& commandContext,
    const FrameContext& frame,
    const Parameters& parameters,
    const PassRuntimeContext& passRuntimeContext) const
{
	GBufferMeshBatchDrawer::DrawOpaqueMeshes(
	    resources,
	    commandContext,
	    frame,
	    parameters,
	    passRuntimeContext,
	    m_runtime,
	    GetDrawParameterMetadata());
}
