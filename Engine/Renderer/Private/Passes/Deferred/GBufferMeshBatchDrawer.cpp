#include "../../PCH.h"

#include "Passes/Deferred/GBufferMeshBatchDrawer.h"

#include "Commands/RenderCommandContext.h"
#include "Frame/Core/FrameContext.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "Meshes/GPUMesh.h"
#include "Meshes/GPUMeshCache.h"
#include "Passes/Core/PassUtilities.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "SceneData/MaterialData.h"
#include "SceneData/RenderSceneData.h"

bool GBufferMeshBatchDrawer::BindMaterial(
    const RenderSceneData& sceneData,
    GBufferPass::DrawParameterInstance& drawParameters,
    std::uint32_t materialSlot)
{
	if (materialSlot >= sceneData.materials.size())
	{
		return false;
	}

	const MaterialData& material =
	    sceneData.materials[materialSlot];
	if (!material.gpuHandle || !material.rasterTextureTable)
	{
		return false;
	}

	drawParameters->PerObjectPS =
	    material.ToPerObjectPSData();
	const RhiDescriptorTableHandle textureTable =
	    material.rasterTextureTable.Table;
	drawParameters->TextureBaseColor = {
	    textureTable,
	    MaterialTextureSlots::BaseColor};
	drawParameters->TextureNormal = {
	    textureTable,
	    MaterialTextureSlots::Normal};
	drawParameters->TextureRoughness = {
	    textureTable,
	    MaterialTextureSlots::Roughness};
	drawParameters->TextureMetallic = {
	    textureTable,
	    MaterialTextureSlots::Metallic};
	drawParameters->TextureOcclusion = {
	    textureTable,
	    MaterialTextureSlots::Occlusion};
	drawParameters->TextureEmissive = {
	    textureTable,
	    MaterialTextureSlots::Emissive};
	drawParameters->TextureSubsurfaceColor = {
	    textureTable,
	    MaterialTextureSlots::SubsurfaceColor};
	drawParameters->TextureSubsurfaceStrength = {
	    textureTable,
	    MaterialTextureSlots::SubsurfaceStrength};
	return true;
}

const GPUMesh* GBufferMeshBatchDrawer::ResolveBatch(
    const RenderSceneData& sceneData,
    const MeshInstanceBatch& batch,
    const GPUMeshCache& meshes) noexcept
{
	if (batch.instanceCount == 0u ||
	    batch.firstInstance >=
	        sceneData.rasterMeshInstanceIndices.size() ||
	    batch.instanceCount >
	        sceneData.rasterMeshInstanceIndices.size() -
	            batch.firstInstance)
	{
		return nullptr;
	}

	const GPUMesh* gpuMesh = meshes.Resolve(batch.Mesh);
	return gpuMesh != nullptr && gpuMesh->IsValid()
	           ? gpuMesh
	           : nullptr;
}

bool GBufferMeshBatchDrawer::HasValidSkinning(
    const RenderSceneData& sceneData,
    const MeshInstanceBatch& batch) noexcept
{
	for (std::uint32_t instanceOffset = 0u;
	     instanceOffset < batch.instanceCount;
	     ++instanceOffset)
	{
		const std::uint32_t drawIndex =
		    sceneData.rasterMeshInstanceIndices[
		        batch.firstInstance + instanceOffset];
		if (drawIndex >= sceneData.meshInstances.size())
		{
			return false;
		}
		const MeshDraw& draw =
		    sceneData.meshInstances[drawIndex];
		if (draw.Geometry.MeshKind !=
		        RenderMeshKind::Skeletal ||
		    draw.Skinning.JointMatrixOffset ==
		        kInvalidMeshInstanceJointMatrixOffset)
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
    const RenderSceneData& sceneData,
    const MeshInstanceBatch& batch,
    const RasterPassPipelineRuntime& runtime)
{
	const bool useTwoSidedPipeline =
	    sceneData.materials[batch.materialSlot].doubleSided && runtime.TwoSidedPipelineState != nullptr;
	return RasterPassPipelineRuntime{
	    runtime.BindingLayout,
	    useTwoSidedPipeline ? *runtime.TwoSidedPipelineState : runtime.PipelineState,
	    runtime.WireframePipelineState,
	    runtime.TwoSidedPipelineState};
}

bool GBufferMeshBatchDrawer::BindBatchPipeline(
    const FrameGraphResourceCommands& resources,
    RenderCommandContext& cmd,
    RenderHardwareInterface& renderHardwareInterface,
    const RasterPassPipelineRuntime& runtime,
    GBufferPass::DrawParameterInstance& drawParameters,
    const GPUMesh& gpuMesh,
    std::uint32_t viewModeIndex)
{
	PassBindingOverrides overrides;
	overrides.SetDescriptorTable("SkinInfluences", gpuMesh.GetSkinInfluencesShaderResourceView());
	overrides.SetDescriptorTable("MorphTargetDeltas", gpuMesh.GetMorphTargetDeltasShaderResourceView());

	return PassUtilities::BindAvailableRasterPassWithRuntime(
	    resources,
	    cmd,
	    &renderHardwareInterface,
	    runtime,
	    drawParameters.GetPassParameterSet(),
	    &overrides,
	    GBufferPass::PassName,
	    false,
	    viewModeIndex);
}

void GBufferMeshBatchDrawer::DrawBatch(
    const FrameGraphResourceCommands& resources,
    RenderCommandContext& cmd,
    const FrameContext& frame,
    const GBufferPass::Parameters& passParameters,
    RenderHardwareInterface& renderHardwareInterface,
    const RasterPassPipelineRuntime& runtime,
    const GBufferPass::DrawParameterMetadata& drawParameterMetadata,
    const GPUMesh& gpuMesh,
    const MeshInstanceBatch& batch,
    std::uint32_t viewModeIndex)
{
	const RenderSceneData& sceneData = frame.sceneData;
	if (batch.meshKind == RenderMeshKind::Skeletal &&
	    (!frame.sceneGpuData->Geometry.HasSkinning() ||
	     !HasValidSkinning(sceneData, batch)))
	{
		return;
	}

	cmd.BindVertexBuffer(gpuMesh.GetVertexBufferView());
	cmd.BindIndexBuffer(gpuMesh.GetIndexBufferView());

	GBufferPass::DrawParameterInstance drawParameters(
	    drawParameterMetadata);
	ConfigureDrawParameters(passParameters, batch, drawParameters);
	if (!BindMaterial(sceneData, drawParameters, batch.materialSlot))
	{
		return;
	}

	const RasterPassPipelineRuntime batchRuntime = ResolveBatchRuntime(sceneData, batch, runtime);
	if (!BindBatchPipeline(
	        resources,
	        cmd,
	        renderHardwareInterface,
	        batchRuntime,
	        drawParameters,
	        gpuMesh,
	        viewModeIndex))
	{
		return;
	}

	cmd.DrawIndexedInstanced(
	    gpuMesh.GetIndexCount(),
	    batch.instanceCount,
	    0,
	    0,
	    0);
}

void GBufferMeshBatchDrawer::DrawOpaqueMeshes(
    const FrameGraphResourceCommands& resources,
    RenderCommandContext& cmd,
    const FrameContext& frame,
    const GBufferPass::Parameters& parameters,
    const PassRuntimeServices& passRuntimeServices,
    const RasterPassPipelineRuntime& runtime,
    const GBufferPass::DrawParameterMetadata& drawParameterMetadata)
{
	if (passRuntimeServices.Meshes == nullptr ||
	    !frame.sceneGpuData->Geometry.HasMeshInstances())
	{
		return;
	}

	const RenderSceneData& sceneData = frame.sceneData;
	const std::uint32_t viewModeIndex =
	    passRuntimeServices.PerFrame.ViewModeIndex;
	for (const MeshInstanceBatch& batch :
	     sceneData.meshInstanceBatches)
	{
		const GPUMesh* gpuMesh =
		    ResolveBatch(
		        sceneData,
		        batch,
		        *passRuntimeServices.Meshes);
		if (gpuMesh == nullptr)
		{
			continue;
		}

		DrawBatch(
		    resources,
		    cmd,
		    frame,
		    parameters,
		    passRuntimeServices.HardwareInterface,
		    runtime,
		    drawParameterMetadata,
		    *gpuMesh,
		    batch,
		    viewModeIndex);
	}
}
