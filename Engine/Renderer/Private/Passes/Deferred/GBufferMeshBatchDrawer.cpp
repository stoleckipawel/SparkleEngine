#include "../../PCH.h"

#include "Passes/Deferred/GBufferMeshBatchDrawer.h"

#include "Commands/RenderCommandContext.h"
#include "Frame/Core/FrameContext.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "FrameGraph/PassRuntimeContext.h"
#include "Meshes/GpuMesh.h"
#include "Meshes/GpuMeshCache.h"
#include "Passes/Core/ShaderPassOperations.h"
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

	const MaterialData& material = sceneData.materials[materialSlot];
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
    const RenderSceneData& sceneData,
    const MeshInstanceBatch& batch,
    const GpuMeshCache& meshes) noexcept
{
	if (batch.instanceCount == 0u || batch.firstInstance >= sceneData.rasterMeshInstanceIndices.size()
	    || batch.instanceCount > sceneData.rasterMeshInstanceIndices.size() - batch.firstInstance)
	{
		return nullptr;
	}

	const GpuMesh* gpuMesh = meshes.Resolve(batch.Mesh);
	return gpuMesh != nullptr && gpuMesh->IsValid() ? gpuMesh : nullptr;
}

bool GBufferMeshBatchDrawer::HasValidSkinning(const RenderSceneData& sceneData, const MeshInstanceBatch& batch) noexcept
{
	for (std::uint32_t instanceOffset = 0u; instanceOffset < batch.instanceCount; ++instanceOffset)
	{
		const std::uint32_t drawIndex = sceneData.rasterMeshInstanceIndices[batch.firstInstance + instanceOffset];
		if (drawIndex >= sceneData.meshInstances.size())
		{
			return false;
		}
		const MeshDraw& draw = sceneData.meshInstances[drawIndex];
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
    const RenderSceneData& sceneData,
    const MeshInstanceBatch& batch,
    const RasterPassPipelineRuntime& runtime)
{
	const bool useTwoSidedPipeline = sceneData.materials[batch.materialSlot].doubleSided && runtime.TwoSidedPipeline != nullptr;
	return RasterPassPipelineRuntime{
	    runtime.BindingLayout,
	    useTwoSidedPipeline ? *runtime.TwoSidedPipeline : runtime.Pipeline,
	    runtime.WireframePipeline,
	    runtime.TwoSidedPipeline};
}

bool GBufferMeshBatchDrawer::BindBatchPipeline(
    const FrameGraphResourceCommands& resources,
    RenderCommandContext& commandContext,
    RenderHardwareInterface& renderHardwareInterface,
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
    RenderCommandContext& commandContext,
    const FrameContext& frame,
    const GBufferPass::Parameters& passParameters,
    RenderHardwareInterface& renderHardwareInterface,
    const RasterPassPipelineRuntime& runtime,
    const GBufferPass::DrawParameterMetadata& drawParameterMetadata,
    const GpuMesh& gpuMesh,
    const MeshInstanceBatch& batch,
    std::uint32_t viewModeIndex)
{
	const RenderSceneData& sceneData = frame.sceneData;
	if (batch.meshKind == RenderMeshKind::Skeletal
	    && (!frame.sceneGpuData->Geometry.HasSkinningBuffers() || !HasValidSkinning(sceneData, batch)))
	{
		return;
	}

	gpuMesh.Bind(commandContext);

	GBufferPass::DrawParameterInstance drawParameters(drawParameterMetadata);
	ConfigureDrawParameters(passParameters, batch, drawParameters);
	if (!BindMaterial(sceneData, drawParameters, batch.materialSlot))
	{
		return;
	}

	const RasterPassPipelineRuntime batchRuntime = ResolveBatchRuntime(sceneData, batch, runtime);
	if (!BindBatchPipeline(resources, commandContext, renderHardwareInterface, batchRuntime, drawParameters, gpuMesh, viewModeIndex))
	{
		return;
	}

	commandContext.DrawIndexedInstanced(gpuMesh.GetIndexCount(), batch.instanceCount, 0, 0, 0);
}

void GBufferMeshBatchDrawer::DrawOpaqueMeshes(
    const FrameGraphResourceCommands& resources,
    RenderCommandContext& commandContext,
    const FrameContext& frame,
    const GBufferPass::Parameters& parameters,
    const PassRuntimeContext& passRuntimeContext,
    const RasterPassPipelineRuntime& runtime,
    const GBufferPass::DrawParameterMetadata& drawParameterMetadata)
{
	if (passRuntimeContext.Meshes == nullptr || !frame.sceneGpuData->Geometry.HasMeshInstanceBuffers())
	{
		return;
	}

	const RenderSceneData& sceneData = frame.sceneData;
	const std::uint32_t viewModeIndex = passRuntimeContext.PerFrame.ViewModeIndex;
	for (const MeshInstanceBatch& batch : sceneData.meshInstanceBatches)
	{
		if (batch.materialClassification != RenderMaterialClassification::Opaque
		    && batch.materialClassification != RenderMaterialClassification::AlphaTested)
		{
			continue;
		}

		const GpuMesh* gpuMesh = ResolveBatch(sceneData, batch, *passRuntimeContext.Meshes);
		if (gpuMesh == nullptr)
		{
			continue;
		}

		DrawBatch(
		    resources,
		    commandContext,
		    frame,
		    parameters,
		    passRuntimeContext.HardwareInterface,
		    runtime,
		    drawParameterMetadata,
		    *gpuMesh,
		    batch,
		    viewModeIndex);
	}
}
