#include "PCH.h"

#include "Scene/GpuScene/RenderSceneGpuBindings.h"

#include "Passes/GBuffer/Raster/GBufferMeshPass.h"
#include "FrameGraph/Execution/PassCommandContext.h"

#include "Commands/RenderCommandContext.h"
#include "Meshes/GpuMesh.h"
#include "Meshes/GpuMeshCache.h"
#include "Passes/Core/ShaderPassOperations.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "Pipeline/GraphicsPipelineMaterialization.h"
#include "Pipeline/RasterPassRenderState.h"
#include "Pipeline/RenderPassRuntimeCache.h"
#include "Passes/GBuffer/Raster/GBufferShaders.h"
#include "Scene/Materials/MaterialData.h"
#include "Scene/Preparation/PreparedRenderScene.h"
#include "View/RenderView.h"

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
    m_gpuMeshCache(gpuMeshCache),
    m_scene(scene),
    m_view(view)
{
}

const GBufferMeshPass::DrawParameterMetadata& GBufferMeshPass::GetDrawParameterMetadata() noexcept
{
	static const DrawParameterMetadata metadata = ShaderParameterStructBuilder<DrawParameters>::BuildMetadata("GBuffer.Draw");
	return metadata;
}

void GBufferMeshPass::PrepareRasterPass(RenderCommandContext& commandContext) const
{
	commandContext.SetViewport(m_view.viewport);
	commandContext.SetScissorRect(m_view.scissorRect);
}

void GBufferMeshPass::BindMaterial(DrawParameterInstance& drawParameters, const PreparedDraw& draw)
{
	drawParameters->Pixel.PerObjectPS = draw.MaterialParameters;
	const RhiDescriptorTableHandle textureTable = draw.MaterialTextures;
	drawParameters->Pixel.TextureBaseColor = {textureTable, MaterialTextureSlots::BaseColor};
	drawParameters->Pixel.TextureNormal = {textureTable, MaterialTextureSlots::Normal};
	drawParameters->Pixel.TextureRoughness = {textureTable, MaterialTextureSlots::Roughness};
	drawParameters->Pixel.TextureMetallic = {textureTable, MaterialTextureSlots::Metallic};
	drawParameters->Pixel.TextureOcclusion = {textureTable, MaterialTextureSlots::Occlusion};
	drawParameters->Pixel.TextureEmissive = {textureTable, MaterialTextureSlots::Emissive};
	drawParameters->Pixel.TextureSubsurfaceColor = {textureTable, MaterialTextureSlots::SubsurfaceColor};
	drawParameters->Pixel.TextureSubsurfaceStrength = {textureTable, MaterialTextureSlots::SubsurfaceStrength};
}

const GpuMesh* GBufferMeshPass::ResolveBatch(const RenderView& view, const MeshInstanceBatch& batch, const GpuMeshCache& meshes) noexcept
{
	if (batch.instanceCount == 0u || batch.firstInstance >= view.rasterPrimitiveIndices.size()
	    || batch.instanceCount > view.rasterPrimitiveIndices.size() - batch.firstInstance)
	{
		return nullptr;
	}

	const GpuMesh* gpuMesh = meshes.Resolve(batch.Mesh);
	return gpuMesh != nullptr && gpuMesh->IsValid() ? gpuMesh : nullptr;
}

bool GBufferMeshPass::HasValidSkinning(
    const PreparedRenderScene& preparedScene,
    const RenderView& view,
    const MeshInstanceBatch& batch) noexcept
{
	for (std::uint32_t instanceOffset = 0u; instanceOffset < batch.instanceCount; ++instanceOffset)
	{
		const std::uint32_t drawIndex = view.rasterPrimitiveIndices[batch.firstInstance + instanceOffset];
		if (drawIndex >= preparedScene.primitives.size())
		{
			return false;
		}
		const MeshDraw& draw = preparedScene.primitives[drawIndex].Draw;
		if (draw.Geometry.MeshKind != RenderMeshKind::Skeletal || draw.Skinning.JointMatrixOffset == kInvalidMeshInstanceJointMatrixOffset)
		{
			return false;
		}
	}
	return true;
}

void GBufferMeshPass::ConfigureDrawParameters(
    const Parameters& passParameters,
    std::uint32_t firstInstance,
    DrawParameterInstance& drawParameters)
{
	drawParameters->Vertex = passParameters.Shader.Vertex;
	drawParameters->Pixel = passParameters.Shader.Pixel;
	drawParameters->Vertex.FirstInstance = firstInstance;
}

RhiRasterizerState GBufferMeshPass::ResolveRasterizerState(const MaterialData& material, const GpuMesh& gpuMesh, bool wireframe) noexcept
{
	return RhiRasterizerState{
	    .FillMode = wireframe ? RhiFillMode::Wireframe : RhiFillMode::Solid,
	    .CullMode = material.doubleSided || wireframe ? ERhiCullMode::None : ERhiCullMode::Back,
	    .FrontFaceWinding = gpuMesh.GetFrontFaceWinding(),
	    .DepthClipEnable = gpuMesh.UsesDepthClipping()};
}

bool GBufferMeshPass::BindBatchPipeline(
    const FrameGraphResourceCommands& resources,
    RenderCommandContext& commandContext,
    DrawParameterInstance& drawParameters,
    const PreparedDraw& draw)
{
	const GpuMesh& mesh = draw.Mesh.get();
	PassBindingOverrides overrides;
	overrides.SetDescriptorTable("SkinInfluences", mesh.GetSkinInfluencesShaderResourceView());
	overrides.SetDescriptorTable("MorphTargetDeltas", mesh.GetMorphTargetDeltasShaderResourceView());

	const RasterPassRuntime runtime{draw.BindingLayout.get(), draw.Pipeline.get()};
	return ShaderPassOperations::BindAvailableRasterPassWithRuntime(
	    resources,
	    commandContext,
	    runtime,
	    drawParameters.GetPassParameterSet(),
	    &overrides,
	    "GBuffer",
	    true);
}

void GBufferMeshPass::DrawBatch(
    const FrameGraphResourceCommands& resources,
    RenderCommandContext& commandContext,
    const Parameters& passParameters,
    const DrawParameterMetadata& drawParameterMetadata,
    const PreparedDraw& draw)
{
	const GpuMesh& mesh = draw.Mesh.get();
	mesh.Bind(commandContext);

	DrawParameterInstance drawParameters(drawParameterMetadata);
	ConfigureDrawParameters(passParameters, draw.FirstInstance, drawParameters);
	BindMaterial(drawParameters, draw);

	if (!BindBatchPipeline(resources, commandContext, drawParameters, draw))
	{
		return;
	}

	commandContext.DrawIndexedInstanced(mesh.GetIndexCount(), draw.InstanceCount, 0, 0, 0);
}

void GBufferMeshPass::Draw(PassCommandContext& context, ParameterInstance& parameters)
{
	const Parameters& passParameters = parameters.GetFields();
	const DrawParameterMetadata& drawParameterMetadata = GetDrawParameterMetadata();
	for (const PreparedDraw& draw : m_preparedDraws)
	{
		DrawBatch(context.Resources, context.Commands, passParameters, drawParameterMetadata, draw);
	}
	m_preparedDraws.clear();
}

void GBufferMeshPass::MaterializePipelines(
    const RenderPassRuntimeCache& runtimeCache,
    const RasterPassRenderState& renderState,
    const GraphicsAttachmentSignature& attachments)
{
	const PreparedRenderScene& preparedScene = m_scene;
	const RenderView& view = m_view;
	const bool wireframe = view.viewMode == RenderViewMode::Wireframe;

	m_preparedDraws.clear();
	if (preparedScene.gpuBindings == nullptr || !preparedScene.gpuBindings->Geometry.HasMeshInstanceBuffers())
	{
		return;
	}
	for (const MeshInstanceBatch& batch : view.meshInstanceBatches)
	{
		if (batch.materialClassification != RenderMaterialClassification::Opaque
		    && batch.materialClassification != RenderMaterialClassification::AlphaTested)
		{
			continue;
		}
		const GpuMesh* const gpuMesh = ResolveBatch(view, batch, m_gpuMeshCache);
		if (gpuMesh == nullptr || batch.materialSlot >= preparedScene.materials.size())
		{
			continue;
		}
		const MaterialData& material = preparedScene.materials[batch.materialSlot];
		if (!material.gpuHandle || !material.rasterTextureTable
		    || (batch.meshKind == RenderMeshKind::Skeletal
		        && (!preparedScene.gpuBindings->Geometry.HasSkinningBuffers() || !HasValidSkinning(preparedScene, view, batch))))
		{
			continue;
		}
		const GraphicsPipelineRequest pipelineRequest = BuildGraphicsPipelineRequest(
		    renderState,
		    ResolveRasterizerState(material, *gpuMesh, wireframe),
		    gpuMesh->GetPrimitiveTopology(),
		    gpuMesh->GetVertexInputDeclaration(),
		    attachments);
		runtimeCache.MaterializeGraphicsShaderRuntime<GBufferVS, GBufferPS>(pipelineRequest);
		const RasterPassRuntime pipeline = runtimeCache.GetGraphicsShaderRuntime<GBufferVS, GBufferPS>(pipelineRequest);
		m_preparedDraws.push_back(
		    PreparedDraw{
		        .Mesh = std::cref(*gpuMesh),
		        .FirstInstance = batch.firstInstance,
		        .InstanceCount = batch.instanceCount,
		        .MaterialParameters = material.ToPerObjectPSData(),
		        .MaterialTextures = material.rasterTextureTable.Table,
		        .BindingLayout = std::ref(pipeline.BindingLayout),
		        .Pipeline = std::ref(pipeline.Pipeline)});
	}
}
