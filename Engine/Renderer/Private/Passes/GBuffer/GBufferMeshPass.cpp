#include "PCH.h"
#include "Passes/GBuffer/GBufferMeshPass.h"

#include "Commands/RenderCommandContext.h"
#include "Core/Public/Diagnostics/Error.h"
#include "View/RenderView.h"
#include "FrameGraph/Execution/PassCommandContext.h"
#include "Passes/GBuffer/GBufferMeshBatchDrawer.h"
#include "Pipeline/GraphicsPipelineMaterialization.h"
#include "Pipeline/RasterPassRenderState.h"
#include "Pipeline/RenderPassRuntimeCache.h"
#include "Scene/Preparation/PreparedRenderScene.h"

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

void GBufferMeshPass::MaterializePipelines(
    const RenderPassRuntimeCache& runtimeCache,
    const RasterPassRenderState& renderState,
    const GraphicsAttachmentSignature& attachments) const
{
	if (m_frameInput == nullptr || !m_frameInput->PreparedScene.has_value() || !m_frameInput->View.has_value())
	{
		throw Diagnostics::Error("GBuffer pass preparation requires the current scene and view.");
	}
	const RenderView& view = m_frameInput->View->get();
	m_meshBatchDrawer->PrepareDrawsAndMaterializePipelines(
	    runtimeCache,
	    m_frameInput->PreparedScene->get(),
	    view,
	    renderState,
	    attachments,
	    m_frameInput->Wireframe);
	m_frameInput->PreparedScene.reset();
	m_frameInput->View.reset();
}

void GBufferMeshPass::Draw(PassCommandContext& context, ParameterInstance& parameters) const
{
	DrawPreparedMeshes(context.Resources, context.Commands, parameters.GetFields());
}

void GBufferMeshPass::PrepareRasterPass(RenderCommandContext& commandContext) const
{
	assert(m_frameInput != nullptr);
	commandContext.SetViewport(m_frameInput->Viewport);
	commandContext.SetScissorRect(m_frameInput->Scissor);
}

void GBufferMeshPass::DrawPreparedMeshes(
    const FrameGraphResourceCommands& resources,
    RenderCommandContext& commandContext,
    const Parameters& parameters) const
{
	m_meshBatchDrawer->DrawPreparedMeshes(resources, commandContext, parameters, GetDrawParameterMetadata());
}
