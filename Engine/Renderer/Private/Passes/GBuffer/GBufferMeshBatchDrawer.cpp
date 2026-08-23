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

bool GBufferMeshBatchDrawer::BindMaterial(
    const PreparedRenderScene& preparedScene,
    GBufferMeshPass::DrawParameterInstance& drawParameters,
    std::uint32_t materialSlot)
{
	if (materialSlot >= preparedScene.materials.size())
	{
		return false;
	}

	const MaterialData& material = preparedScene.materials[materialSlot];
	if (!material.gpuHandle || !material.rasterTextureTable)
	{
		return false;
	}

	drawParameters->Pixel.PerObjectPS = material.ToPerObjectPSData();
	const RhiDescriptorTableHandle textureTable = material.rasterTextureTable.Table;
	drawParameters->Pixel.TextureBaseColor = {textureTable, MaterialTextureSlots::BaseColor};
	drawParameters->Pixel.TextureNormal = {textureTable, MaterialTextureSlots::Normal};
	drawParameters->Pixel.TextureRoughness = {textureTable, MaterialTextureSlots::Roughness};
	drawParameters->Pixel.TextureMetallic = {textureTable, MaterialTextureSlots::Metallic};
	drawParameters->Pixel.TextureOcclusion = {textureTable, MaterialTextureSlots::Occlusion};
	drawParameters->Pixel.TextureEmissive = {textureTable, MaterialTextureSlots::Emissive};
	drawParameters->Pixel.TextureSubsurfaceColor = {textureTable, MaterialTextureSlots::SubsurfaceColor};
	drawParameters->Pixel.TextureSubsurfaceStrength = {textureTable, MaterialTextureSlots::SubsurfaceStrength};
	return true;
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
    const MeshInstanceBatch& batch,
    GBufferMeshPass::DrawParameterInstance& drawParameters)
{
	drawParameters->Vertex = passParameters.Shader.Vertex;
	drawParameters->Pixel = passParameters.Shader.Pixel;
	drawParameters->Vertex.MeshInstanceDraw = MeshInstanceDrawConstantBufferData{.FirstInstance = batch.firstInstance};
}

RhiRasterizerState GBufferMeshBatchDrawer::ResolveRasterizerState(
    const PreparedRenderScene& preparedScene,
    const MeshInstanceBatch& batch,
    const GpuMesh& gpuMesh,
    bool wireframe) noexcept
{
	const bool twoSided = batch.materialSlot < preparedScene.materials.size()
	    && preparedScene.materials[batch.materialSlot].doubleSided;
	return RhiRasterizerState{
	    .FillMode = wireframe ? RhiFillMode::Wireframe : RhiFillMode::Solid,
	    .CullMode = twoSided || wireframe ? ERhiCullMode::None : ERhiCullMode::Back,
	    .FrontFaceWinding = gpuMesh.GetFrontFaceWinding(),
	    .DepthClipEnable = gpuMesh.UsesDepthClipping()};
}

bool GBufferMeshBatchDrawer::BindBatchPipeline(
    const FrameGraphResourceCommands& resources,
    RenderCommandContext& commandContext,
    const RenderPassRuntimeCache& runtimeCache,
    const RasterPassRenderState& renderState,
    const GraphicsAttachmentSignature& attachments,
    const PreparedRenderScene& preparedScene,
    const MeshInstanceBatch& batch,
    GBufferMeshPass::DrawParameterInstance& drawParameters,
    const GpuMesh& gpuMesh,
    bool wireframe)
{
	PassBindingOverrides overrides;
	overrides.SetDescriptorTable("SkinInfluences", gpuMesh.GetSkinInfluencesShaderResourceView());
	overrides.SetDescriptorTable("MorphTargetDeltas", gpuMesh.GetMorphTargetDeltasShaderResourceView());

	const RasterPassRuntime runtime = runtimeCache.GetGraphicsShaderRuntime<GBufferVS, GBufferPS>(
	    renderState,
	    ResolveRasterizerState(preparedScene, batch, gpuMesh, wireframe),
	    gpuMesh.GetPrimitiveTopology(),
	    gpuMesh.GetVertexInputDeclaration(),
	    attachments);
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
    const PreparedRenderScene& preparedScene,
    const RenderView& view,
    const GBufferMeshPass::Parameters& passParameters,
    const RenderPassRuntimeCache& runtimeCache,
    const RasterPassRenderState& renderState,
    const GraphicsAttachmentSignature& attachments,
    const GBufferMeshPass::DrawParameterMetadata& drawParameterMetadata,
    const GpuMesh& gpuMesh,
    const MeshInstanceBatch& batch,
    bool wireframe)
{
	if (batch.meshKind == RenderMeshKind::Skeletal
	    && (!preparedScene.gpuBindings->Geometry.HasSkinningBuffers() || !HasValidSkinning(preparedScene, view, batch)))
	{
		return;
	}

	gpuMesh.Bind(commandContext);

	GBufferMeshPass::DrawParameterInstance drawParameters(drawParameterMetadata);
	ConfigureDrawParameters(passParameters, batch, drawParameters);
	if (!BindMaterial(preparedScene, drawParameters, batch.materialSlot))
	{
		return;
	}

	if (!BindBatchPipeline(
	        resources,
	        commandContext,
	        runtimeCache,
	        renderState,
	        attachments,
	        preparedScene,
	        batch,
	        drawParameters,
	        gpuMesh,
	        wireframe))
	{
		return;
	}

	commandContext.DrawIndexedInstanced(gpuMesh.GetIndexCount(), batch.instanceCount, 0, 0, 0);
}

void GBufferMeshBatchDrawer::DrawOpaqueMeshes(
    const FrameGraphResourceCommands& resources,
    RenderCommandContext& commandContext,
    const PreparedRenderScene& preparedScene,
    const RenderView& view,
    const GBufferMeshPass::Parameters& parameters,
    const RenderPassRuntimeCache& runtimeCache,
    const RasterPassRenderState& renderState,
    const GraphicsAttachmentSignature& attachments,
    bool wireframe,
    const GBufferMeshPass::DrawParameterMetadata& drawParameterMetadata) const
{
	if (!preparedScene.gpuBindings->Geometry.HasMeshInstanceBuffers())
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

		const GpuMesh* gpuMesh = ResolveBatch(view, batch, m_gpuMeshCache);
		if (gpuMesh == nullptr)
		{
			continue;
		}

		DrawBatch(
		    resources,
		    commandContext,
		    preparedScene,
		    view,
		    parameters,
		    runtimeCache,
		    renderState,
		    attachments,
		    drawParameterMetadata,
		    *gpuMesh,
		    batch,
		    wireframe);
	}
}

void GBufferMeshBatchDrawer::MaterializePipelines(
    const RenderPassRuntimeCache& runtimeCache,
    const PreparedRenderScene& preparedScene,
    const RenderView& view,
    const RasterPassRenderState& renderState,
    const GraphicsAttachmentSignature& attachments,
    bool wireframe) const
{
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
		runtimeCache.MaterializeGraphicsShaderRuntime<GBufferVS, GBufferPS>(
		    renderState,
		    ResolveRasterizerState(preparedScene, batch, *gpuMesh, wireframe),
		    gpuMesh->GetPrimitiveTopology(),
		    gpuMesh->GetVertexInputDeclaration(),
		    attachments);
	}
}
