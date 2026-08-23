#include "../../PCH.h"

#include "Scene/GpuScene/RenderSceneGpuBindings.h"

#include "Passes/GBuffer/GBufferMeshBatchDrawer.h"

#include "Commands/RenderCommandContext.h"
#include "Meshes/GpuMesh.h"
#include "Meshes/GpuMeshCache.h"
#include "Passes/Core/ShaderPassOperations.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "Pipeline/GraphicsPipelineMaterialization.h"
#include "Pipeline/RasterPassRenderState.h"
#include "Pipeline/RenderPassRuntimeCache.h"
#include "Passes/GBuffer/GBufferShaders.h"
#include "Scene/Materials/MaterialData.h"
#include "Scene/Preparation/PreparedRenderScene.h"
#include "View/RenderView.h"

GBufferMeshBatchDrawer::~GBufferMeshBatchDrawer() noexcept = default;

void GBufferMeshBatchDrawer::BindMaterial(GBufferMeshPass::DrawParameterInstance& drawParameters, const PreparedDraw& draw)
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

const GpuMesh* GBufferMeshBatchDrawer::ResolveBatch(
    const RenderView& view,
    const MeshInstanceBatch& batch,
    const GpuMeshCache& meshes) noexcept
{
	if (batch.instanceCount == 0u || batch.firstInstance >= view.rasterPrimitiveIndices.size()
	    || batch.instanceCount > view.rasterPrimitiveIndices.size() - batch.firstInstance)
	{
		return nullptr;
	}

	const GpuMesh* gpuMesh = meshes.Resolve(batch.Mesh);
	return gpuMesh != nullptr && gpuMesh->IsValid() ? gpuMesh : nullptr;
}

bool GBufferMeshBatchDrawer::HasValidSkinning(
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

void GBufferMeshBatchDrawer::ConfigureDrawParameters(
    const GBufferMeshPass::Parameters& passParameters,
    std::uint32_t firstInstance,
    GBufferMeshPass::DrawParameterInstance& drawParameters)
{
	drawParameters->Vertex = passParameters.Shader.Vertex;
	drawParameters->Pixel = passParameters.Shader.Pixel;
	drawParameters->Vertex.MeshInstanceDraw = MeshInstanceDrawConstantBufferData{.FirstInstance = firstInstance};
}

RhiRasterizerState GBufferMeshBatchDrawer::ResolveRasterizerState(
    const MaterialData& material,
    const GpuMesh& gpuMesh,
    bool wireframe) noexcept
{
	return RhiRasterizerState{
	    .FillMode = wireframe ? RhiFillMode::Wireframe : RhiFillMode::Solid,
	    .CullMode = material.doubleSided || wireframe ? ERhiCullMode::None : ERhiCullMode::Back,
	    .FrontFaceWinding = gpuMesh.GetFrontFaceWinding(),
	    .DepthClipEnable = gpuMesh.UsesDepthClipping()};
}

bool GBufferMeshBatchDrawer::BindBatchPipeline(
    const FrameGraphResourceCommands& resources,
    RenderCommandContext& commandContext,
    GBufferMeshPass::DrawParameterInstance& drawParameters,
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

void GBufferMeshBatchDrawer::DrawBatch(
    const FrameGraphResourceCommands& resources,
    RenderCommandContext& commandContext,
    const GBufferMeshPass::Parameters& passParameters,
    const GBufferMeshPass::DrawParameterMetadata& drawParameterMetadata,
    const PreparedDraw& draw)
{
	const GpuMesh& mesh = draw.Mesh.get();
	mesh.Bind(commandContext);

	GBufferMeshPass::DrawParameterInstance drawParameters(drawParameterMetadata);
	ConfigureDrawParameters(passParameters, draw.FirstInstance, drawParameters);
	BindMaterial(drawParameters, draw);

	if (!BindBatchPipeline(resources, commandContext, drawParameters, draw))
	{
		return;
	}

	commandContext.DrawIndexedInstanced(mesh.GetIndexCount(), draw.InstanceCount, 0, 0, 0);
}

void GBufferMeshBatchDrawer::DrawPreparedMeshes(
    const FrameGraphResourceCommands& resources,
    RenderCommandContext& commandContext,
    const GBufferMeshPass::Parameters& parameters,
    const GBufferMeshPass::DrawParameterMetadata& drawParameterMetadata)
{
	for (const PreparedDraw& draw : m_preparedDraws)
	{
		DrawBatch(resources, commandContext, parameters, drawParameterMetadata, draw);
	}
	m_preparedDraws.clear();
}

void GBufferMeshBatchDrawer::PrepareDrawsAndMaterializePipelines(
    const RenderPassRuntimeCache& runtimeCache,
    const PreparedRenderScene& preparedScene,
    const RenderView& view,
    const RasterPassRenderState& renderState,
    const GraphicsAttachmentSignature& attachments,
    bool wireframe)
{
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
