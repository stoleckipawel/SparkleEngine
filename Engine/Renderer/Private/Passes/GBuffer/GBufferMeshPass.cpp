#include "PCH.h"
#include "Passes/GBuffer/GBufferMeshPass.h"

#include "Commands/RenderCommandContext.h"
#include "View/RenderView.h"
#include "FrameGraph/Execution/PassCommandContext.h"
#include "Passes/GBuffer/GBufferMeshBatchDrawer.h"
#include "Passes/Core/ShaderPassOperations.h"
#include "Scene/Preparation/PreparedRenderScene.h"

#include "Pipeline/PassPipelineRuntime.h"

#include <array>
#include <cassert>

void GBufferShaderParameters::Describe(ShaderParameterStructBuilder<GBufferShaderParameters>& builder)
{
	builder.Include(&GBufferShaderParameters::Vertex, ShaderStageVisibility::Vertex);
	builder.Include(&GBufferShaderParameters::Pixel, ShaderStageVisibility::Pixel);
}

void GBufferGraphParameters::Describe(ShaderParameterStructBuilder<GBufferGraphParameters>& builder)
{
	builder.RenderTarget("BaseColor", &GBufferGraphParameters::BaseColor, ShaderStageVisibility::AllGraphics);
	builder.RenderTarget("Normal", &GBufferGraphParameters::Normal, ShaderStageVisibility::AllGraphics);
	builder.RenderTarget("Material", &GBufferGraphParameters::Material, ShaderStageVisibility::AllGraphics);
	builder.RenderTarget("Emissive", &GBufferGraphParameters::Emissive, ShaderStageVisibility::AllGraphics);
	builder.RenderTarget("Subsurface", &GBufferGraphParameters::Subsurface, ShaderStageVisibility::AllGraphics);
	builder.RenderTarget("MotionVector", &GBufferGraphParameters::MotionVector, ShaderStageVisibility::AllGraphics);
	builder.DepthTarget("DeviceZ", &GBufferGraphParameters::DeviceZ, ShaderStageVisibility::AllGraphics);
	builder.Include(&GBufferGraphParameters::Shader);
}

GBufferMeshPass::GBufferMeshPass(GpuMeshCache& gpuMeshCache, const std::shared_ptr<GBufferMeshPassInput>& frameInput) noexcept :
    m_meshBatchDrawer(std::make_shared<GBufferMeshBatchDrawer>(gpuMeshCache)),
    m_frameInput(frameInput)
{
}

GBufferMeshPass::~GBufferMeshPass() noexcept = default;

const GBufferMeshPass::DrawParameterMetadata& GBufferMeshPass::GetDrawParameterMetadata() noexcept
{
	static const DrawParameterMetadata metadata = []
	{
		return ShaderParameterStructBuilder<DrawParameters>::BuildMetadata("GBuffer.Draw");
	}();

	return metadata;
}

void GBufferMeshPass::Draw(PassCommandContext& context, ParameterInstance& parameters, const RasterPassPipelineRuntime& runtime) const
{
	assert(m_frameInput != nullptr && m_frameInput->PreparedScene.has_value() && m_frameInput->View.has_value());
	const PreparedRenderScene& preparedScene = m_frameInput->PreparedScene->get();
	const RenderView& view = m_frameInput->View->get();
	ConfigurePipeline(context.Commands, view);
	PrepareTargets(context, parameters.GetFields());
	BindPassResources(context.Resources, context.Commands, parameters, view, runtime);
	DrawOpaqueMeshes(context.Resources, context.Commands, preparedScene, view, parameters.GetFields(), runtime);
}

void GBufferMeshPass::PrepareTargets(PassCommandContext& context, const GBufferMeshPass::Parameters& parameters) const
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

void GBufferMeshPass::ConfigurePipeline(RenderCommandContext& commandContext, const RenderView& view) const
{
	commandContext.SetViewport(view.viewport);
	commandContext.SetScissorRect(view.scissorRect);
	commandContext.SetPrimitiveTopology(RhiPrimitiveTopology::TriangleList);
}

void GBufferMeshPass::BindPassResources(
    const FrameGraphResourceCommands& resources,
    RenderCommandContext& commandContext,
    const ParameterInstance& parameters,
    const RenderView& view,
    const RasterPassPipelineRuntime& runtime) const
{
	const bool bound = ShaderPassOperations::BindAvailableRasterPassWithRuntime(
	    resources,
	    commandContext,
	    runtime,
	    parameters.GetPassParameterSet(),
	    nullptr,
	    "GBuffer",
	    true,
	    view.uniform.ViewModeIndex);
	assert(bound);
}

void GBufferMeshPass::DrawOpaqueMeshes(
    const FrameGraphResourceCommands& resources,
    RenderCommandContext& commandContext,
    const PreparedRenderScene& preparedScene,
    const RenderView& view,
    const Parameters& parameters,
    const RasterPassPipelineRuntime& runtime) const
{
	m_meshBatchDrawer->DrawOpaqueMeshes(resources, commandContext, preparedScene, view, parameters, runtime, GetDrawParameterMetadata());
}
