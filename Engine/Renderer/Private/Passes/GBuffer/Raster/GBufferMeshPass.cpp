#include "PCH.h"
#include "Passes/GBuffer/Raster/GBufferMeshPass.h"

#include "Commands/RenderCommandContext.h"
#include "Core/Public/Diagnostics/Error.h"
#include "View/RenderView.h"
#include "FrameGraph/Execution/PassCommandContext.h"
#include "Passes/GBuffer/Raster/GBufferMeshBatchDrawer.h"
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
	builder.RenderTarget("WorldNormal", &GBufferGraphParameters::WorldNormal, ShaderStageVisibility::AllGraphics);
	builder.RenderTarget("WorldTangent", &GBufferGraphParameters::WorldTangent, ShaderStageVisibility::AllGraphics);
	builder.RenderTarget("Material", &GBufferGraphParameters::Material, ShaderStageVisibility::AllGraphics);
	builder.RenderTarget("Emissive", &GBufferGraphParameters::Emissive, ShaderStageVisibility::AllGraphics);
	builder.RenderTarget("Subsurface", &GBufferGraphParameters::Subsurface, ShaderStageVisibility::AllGraphics);
	builder.RenderTarget("MotionVector", &GBufferGraphParameters::MotionVector, ShaderStageVisibility::AllGraphics);
	builder.DepthTarget("DeviceZ", &GBufferGraphParameters::DeviceZ, ShaderStageVisibility::AllGraphics);
	builder.Include(&GBufferGraphParameters::Shader);
}

GBufferMeshPass::GBufferMeshPass(GpuMeshCache& gpuMeshCache, const PreparedRenderScene& scene, const RenderView& view) noexcept :
    m_meshBatchDrawer(std::make_shared<GBufferMeshBatchDrawer>(gpuMeshCache)),
    m_scene(scene),
    m_view(view)
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
	const RenderView& view = m_view.get();
	m_meshBatchDrawer->PrepareDrawsAndMaterializePipelines(
	    runtimeCache,
	    m_scene.get(),
	    view,
	    renderState,
	    attachments,
	    view.viewMode == RenderViewMode::Wireframe);
}

void GBufferMeshPass::Draw(PassCommandContext& context, ParameterInstance& parameters) const
{
	DrawPreparedMeshes(context.Resources, context.Commands, parameters.GetFields());
}

void GBufferMeshPass::PrepareRasterPass(RenderCommandContext& commandContext) const
{
	commandContext.SetViewport(m_view.get().viewport);
	commandContext.SetScissorRect(m_view.get().scissorRect);
}

void GBufferMeshPass::DrawPreparedMeshes(
    const FrameGraphResourceCommands& resources,
    RenderCommandContext& commandContext,
    const Parameters& parameters) const
{
	m_meshBatchDrawer->DrawPreparedMeshes(resources, commandContext, parameters, GetDrawParameterMetadata());
}
