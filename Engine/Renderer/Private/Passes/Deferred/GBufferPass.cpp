#include "PCH.h"
#include "Passes/Deferred/GBufferPass.h"

#include "Config/DepthConvention.h"
#include "Commands/RenderCommandContext.h"
#include "Diagnostics/PassExecutionDiagnostics.h"
#include "Core/Public/Diagnostics/Logger.h"
#include "View/RenderView.h"
#include "Frame/Deferred/GBufferFormats.h"
#include "FrameGraph/Execution/PassCommandContext.h"
#include "Passes/Deferred/GBufferMeshBatchDrawer.h"
#include "Passes/Core/ShaderPassOperations.h"
#include "Passes/Core/RasterPassOperations.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "Scene/Preparation/PreparedRenderScene.h"
#include "Scene/Materials/MaterialData.h"
#include "Renderer/Public/SceneData/MeshDraw.h"
#include "Meshes/GpuMesh.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

#include "RHI/Public/Bindings/RenderBindingSet.h"
#include "RHI/Public/Samplers/RhiSamplerDesc.h"

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
	builder.Uniform("View", &GBufferPassParameters::View, ShaderStageVisibility::Pixel);
	builder.Uniform("ViewCamera", &GBufferPassParameters::ViewCamera, ShaderStageVisibility::Vertex);
	builder.Uniform("ViewTemporal", &GBufferPassParameters::ViewTemporal, ShaderStageVisibility::Vertex | ShaderStageVisibility::Pixel);
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

GBufferPass::GBufferPass(
    const RasterPassPipelineRuntime& runtime,
    GpuMeshCache& gpuMeshCache,
    const std::shared_ptr<GBufferPassFrameInput>& frameInput) noexcept :
    m_runtime(runtime),
    m_meshBatchDrawer(std::make_shared<GBufferMeshBatchDrawer>(gpuMeshCache)),
    m_frameInput(frameInput)
{
}

GBufferPass::~GBufferPass() noexcept = default;

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

void GBufferPass::Execute(PassCommandContext& context, ParameterInstance& parameters) const
{
	assert(m_frameInput != nullptr && m_frameInput->PreparedScene.has_value() && m_frameInput->View.has_value());
	const PreparedRenderScene& preparedScene = m_frameInput->PreparedScene->get();
	const RenderView& view = m_frameInput->View->get();
	ConfigurePipeline(context.Commands, view);
	PrepareTargets(context, parameters.GetFields());
	BindPassResources(context.Resources, context.Commands, parameters, view);
	DrawOpaqueMeshes(context.Resources, context.Commands, preparedScene, view, parameters.GetFields());
}

void GBufferPass::PrepareTargets(PassCommandContext& context, const GBufferPass::Parameters& parameters) const
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

void GBufferPass::ConfigurePipeline(RenderCommandContext& commandContext, const RenderView& view) const
{
	commandContext.SetViewport(view.viewport);
	commandContext.SetScissorRect(view.scissorRect);
	commandContext.SetPrimitiveTopology(RhiPrimitiveTopology::TriangleList);
}

void GBufferPass::BindPassResources(
    const FrameGraphResourceCommands& resources,
    RenderCommandContext& commandContext,
    const ParameterInstance& parameters,
    const RenderView& view) const
{
	const bool bound = ShaderPassOperations::BindAvailableRasterPassWithRuntime(
	    resources,
	    commandContext,
	    m_runtime,
	    parameters.GetPassParameterSet(),
	    nullptr,
	    PassName,
	    true,
	    view.uniform.ViewModeIndex);
	assert(bound);
}

void GBufferPass::DrawOpaqueMeshes(
    const FrameGraphResourceCommands& resources,
    RenderCommandContext& commandContext,
    const PreparedRenderScene& preparedScene,
    const RenderView& view,
    const Parameters& parameters) const
{
	m_meshBatchDrawer->DrawOpaqueMeshes(resources, commandContext, preparedScene, view, parameters, m_runtime, GetDrawParameterMetadata());
}
