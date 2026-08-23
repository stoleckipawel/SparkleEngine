#include "../../PCH.h"

#include "Scene/GpuScene/RenderSceneGpuBindings.h"

#include "Passes/GBuffer/GBufferMeshBatchDrawer.h"

#include "Commands/RenderCommandContext.h"
#include "Meshes/GpuMesh.h"
#include "Meshes/GpuMeshCache.h"
#include "Passes/Core/ShaderPassOperations.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "Scene/Materials/MaterialData.h"
#include "Scene/Preparation/PreparedRenderScene.h"
#include "View/RenderView.h"

bool GBufferMeshBatchDrawer::BindMaterial(
    const PreparedRenderScene& preparedScene,
    GBufferPass::DrawParameterInstance& drawParameters,
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

	drawParameters->PerObjectPS = material.ToPerObjectPSData();
	const RhiDescriptorTableHandle textureTable = material.rasterTextureTable.Table;
	drawParameters->TextureBaseColor = {textureTable, MaterialTextureSlots::BaseColor};
	drawParameters->TextureNormal = {textureTable, MaterialTextureSlots::Normal};
	drawParameters->TextureRoughness = {textureTable, MaterialTextureSlots::Roughness};
	drawParameters->TextureMetallic = {textureTable, MaterialTextureSlots::Metallic};
	drawParameters->TextureOcclusion = {textureTable, MaterialTextureSlots::Occlusion};
	drawParameters->TextureEmissive = {textureTable, MaterialTextureSlots::Emissive};
	drawParameters->TextureSubsurfaceColor = {textureTable, MaterialTextureSlots::SubsurfaceColor};
	drawParameters->TextureSubsurfaceStrength = {textureTable, MaterialTextureSlots::SubsurfaceStrength};
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
    const GBufferPass::Parameters& passParameters,
    const MeshInstanceBatch& batch,
    GBufferPass::DrawParameterInstance& drawParameters)
{
	drawParameters->MeshInstanceDraw = MeshInstanceDrawConstantBufferData{.FirstInstance = batch.firstInstance};
	drawParameters->MeshInstances = passParameters.MeshInstances;
	drawParameters->MeshInstanceSlots = passParameters.MeshInstanceSlots;
	drawParameters->JointMatrices = passParameters.JointMatrices;
	drawParameters->PreviousJointMatrices = passParameters.PreviousJointMatrices;
	drawParameters->MorphWeights = passParameters.MorphWeights;
	drawParameters->PreviousMorphWeights = passParameters.PreviousMorphWeights;
}

RasterPassPipelineRuntime GBufferMeshBatchDrawer::ResolveBatchRuntime(
    const PreparedRenderScene& preparedScene,
    const MeshInstanceBatch& batch,
    const RasterPassPipelineRuntime& runtime)
{
	const bool useTwoSidedPipeline = preparedScene.materials[batch.materialSlot].doubleSided && runtime.TwoSidedPipeline != nullptr;
	return RasterPassPipelineRuntime{
	    runtime.BindingLayout,
	    useTwoSidedPipeline ? *runtime.TwoSidedPipeline : runtime.Pipeline,
	    runtime.WireframePipeline,
	    runtime.TwoSidedPipeline};
}

bool GBufferMeshBatchDrawer::BindBatchPipeline(
    const FrameGraphResourceCommands& resources,
    RenderCommandContext& commandContext,
    const RasterPassPipelineRuntime& runtime,
    GBufferPass::DrawParameterInstance& drawParameters,
    const GpuMesh& gpuMesh,
    std::uint32_t viewModeIndex)
{
	PassBindingOverrides overrides;
	overrides.SetDescriptorTable("SkinInfluences", gpuMesh.GetSkinInfluencesShaderResourceView());
	overrides.SetDescriptorTable("MorphTargetDeltas", gpuMesh.GetMorphTargetDeltasShaderResourceView());

	return ShaderPassOperations::BindAvailableRasterPassWithRuntime(
	    resources,
	    commandContext,
	    runtime,
	    drawParameters.GetPassParameterSet(),
	    &overrides,
	    GBufferPass::PassName,
	    false,
	    viewModeIndex);
}

void GBufferMeshBatchDrawer::DrawBatch(
    const FrameGraphResourceCommands& resources,
    RenderCommandContext& commandContext,
    const PreparedRenderScene& preparedScene,
    const RenderView& view,
    const GBufferPass::Parameters& passParameters,
    const RasterPassPipelineRuntime& runtime,
    const GBufferPass::DrawParameterMetadata& drawParameterMetadata,
    const GpuMesh& gpuMesh,
    const MeshInstanceBatch& batch,
    std::uint32_t viewModeIndex)
{
	if (batch.meshKind == RenderMeshKind::Skeletal
	    && (!preparedScene.gpuBindings->Geometry.HasSkinningBuffers() || !HasValidSkinning(preparedScene, view, batch)))
	{
		return;
	}

	gpuMesh.Bind(commandContext);

	GBufferPass::DrawParameterInstance drawParameters(drawParameterMetadata);
	ConfigureDrawParameters(passParameters, batch, drawParameters);
	if (!BindMaterial(preparedScene, drawParameters, batch.materialSlot))
	{
		return;
	}

	const RasterPassPipelineRuntime batchRuntime = ResolveBatchRuntime(preparedScene, batch, runtime);
	if (!BindBatchPipeline(resources, commandContext, batchRuntime, drawParameters, gpuMesh, viewModeIndex))
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
    const GBufferPass::Parameters& parameters,
    const RasterPassPipelineRuntime& runtime,
    const GBufferPass::DrawParameterMetadata& drawParameterMetadata) const
{
	if (!preparedScene.gpuBindings->Geometry.HasMeshInstanceBuffers())
	{
		return;
	}

	const std::uint32_t viewModeIndex = view.uniform.ViewModeIndex;
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
		    runtime,
		    drawParameterMetadata,
		    *gpuMesh,
		    batch,
		    viewModeIndex);
	}
}
