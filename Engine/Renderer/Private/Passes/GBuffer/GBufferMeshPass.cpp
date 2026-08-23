#include "PCH.h"
#include "Passes/GBuffer/GBufferMeshPass.h"

#include "Commands/RenderCommandContext.h"
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
	assert(m_frameInput != nullptr && m_frameInput->PreparedScene.has_value() && m_frameInput->View.has_value());
	const RenderView& view = m_frameInput->View->get();
	m_meshBatchDrawer->MaterializePipelines(
	    runtimeCache,
	    m_frameInput->PreparedScene->get(),
	    view,
	    renderState,
	    attachments,
	    m_frameInput->Wireframe);
}

void GBufferMeshPass::Draw(
    PassCommandContext& context,
    ParameterInstance& parameters,
    const RenderPassRuntimeCache& runtimeCache,
    const RasterPassRenderState& renderState,
    const GraphicsAttachmentSignature& attachments) const
{
	assert(m_frameInput != nullptr && m_frameInput->PreparedScene.has_value() && m_frameInput->View.has_value());
	const PreparedRenderScene& preparedScene = m_frameInput->PreparedScene->get();
	const RenderView& view = m_frameInput->View->get();
	DrawOpaqueMeshes(
	    context.Resources,
	    context.Commands,
	    preparedScene,
	    view,
	    parameters.GetFields(),
	    runtimeCache,
	    renderState,
	    attachments,
	    m_frameInput->Wireframe);
}

void GBufferMeshPass::PrepareRasterPass(RenderCommandContext& commandContext) const
{
	assert(m_frameInput != nullptr && m_frameInput->View.has_value());
	const RenderView& view = m_frameInput->View->get();
	commandContext.SetViewport(view.viewport);
	commandContext.SetScissorRect(view.scissorRect);
}

void GBufferMeshPass::DrawOpaqueMeshes(
    const FrameGraphResourceCommands& resources,
    RenderCommandContext& commandContext,
    const PreparedRenderScene& preparedScene,
    const RenderView& view,
    const Parameters& parameters,
    const RenderPassRuntimeCache& runtimeCache,
    const RasterPassRenderState& renderState,
    const GraphicsAttachmentSignature& attachments,
    bool wireframe) const
{
	m_meshBatchDrawer->DrawOpaqueMeshes(
	    resources,
	    commandContext,
	    preparedScene,
	    view,
	    parameters,
	    runtimeCache,
	    renderState,
	    attachments,
	    wireframe,
	    GetDrawParameterMetadata());
}
