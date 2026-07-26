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

#include <cassert>

static const auto g_gbufferMeshBatchDrawerLogger = Logging::GetOrCreateLogger("Renderer.GBufferMeshBatchDrawer");

class GBufferMeshBatchDrawerOperations final
{
  public:
	static constexpr const char* ToMeshKindLabel(RenderMeshKind meshKind) noexcept
	{
		return meshKind == RenderMeshKind::Skeletal ? "skinned" : "static";
	}

	static bool BindMaterial(const RenderSceneData& sceneData, GBufferPass::DrawParameterInstance& drawParameters, std::uint32_t materialSlot)
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

	static const GPUMesh* ResolveValidBatch(
	    const RenderSceneData& sceneData,
	    const MeshInstanceBatch& batch,
	    std::size_t batchIndex,
	    const GPUMeshCache& meshes)
	{
		if (batch.instanceCount == 0)
		{
			SPDLOG_LOGGER_WARN(
			    g_gbufferMeshBatchDrawerLogger,
			    "GBuffer: {} batch {} is empty; skipped.",
			    ToMeshKindLabel(batch.meshKind),
			    batchIndex);
			return nullptr;
		}

		if (batch.firstInstance >= sceneData.meshInstances.size() ||
		    batch.instanceCount > sceneData.meshInstances.size() - batch.firstInstance)
		{
			SPDLOG_LOGGER_WARN(
			    g_gbufferMeshBatchDrawerLogger,
			    "GBuffer: {} batch {} references instance range [{}..{}) outside {} uploaded instances; skipped.",
			    ToMeshKindLabel(batch.meshKind),
			    batchIndex,
			    batch.firstInstance,
			    batch.firstInstance + batch.instanceCount,
			    sceneData.meshInstances.size());
			return nullptr;
		}

		const GPUMesh* gpuMesh = meshes.Resolve(batch.Mesh);
		if (gpuMesh == nullptr || !gpuMesh->IsValid())
		{
			SPDLOG_LOGGER_WARN(
			    g_gbufferMeshBatchDrawerLogger,
			    "GBuffer: {} batch {} has no valid GPU mesh; skipped.",
			    ToMeshKindLabel(batch.meshKind),
			    batchIndex);
			return nullptr;
		}

		return gpuMesh;
	}

	static bool HasValidSkinnedInstanceRange(const RenderSceneData& sceneData, const MeshInstanceBatch& batch) noexcept
	{
		for (std::uint32_t instanceOffset = 0; instanceOffset < batch.instanceCount; ++instanceOffset)
		{
			const MeshDraw& draw = sceneData.meshInstances[batch.firstInstance + instanceOffset];
			if (draw.Geometry.MeshKind != RenderMeshKind::Skeletal ||
			    draw.Skinning.JointMatrixOffset == kInvalidMeshInstanceJointMatrixOffset)
			{
				return false;
			}
		}
		return true;
	}

	static bool DrawBatch(
	    const FrameGraphResourceCommands& resources,
	    RenderCommandContext& cmd,
	    const FrameContext& frame,
	    const GBufferPass::Parameters& passParameters,
	    RenderHardwareInterface& renderHardwareInterface,
	    const RasterPassPipelineRuntime& runtime,
	    const GBufferPass::DrawParameterMetadata& drawParameterMetadata,
	    const GPUMesh& gpuMesh,
	    const MeshInstanceBatch& batch,
	    std::size_t batchIndex,
	    std::uint32_t viewModeIndex)
	{
		const RenderSceneData& sceneData = frame.sceneData;
		cmd.BindVertexBuffer(gpuMesh.GetVertexBufferView());
		cmd.BindIndexBuffer(gpuMesh.GetIndexBufferView());

		GBufferPass::DrawParameterInstance drawParameters(drawParameterMetadata);
		drawParameters->MeshInstanceDraw = MeshInstanceDrawConstantBufferData{.FirstInstance = batch.firstInstance};
		drawParameters->MeshInstances = passParameters.MeshInstances;
		drawParameters->MeshInstanceSlots = passParameters.MeshInstanceSlots;
		drawParameters->JointMatrices = passParameters.JointMatrices;
		drawParameters->PreviousJointMatrices = passParameters.PreviousJointMatrices;
		drawParameters->MorphWeights = passParameters.MorphWeights;
		drawParameters->PreviousMorphWeights = passParameters.PreviousMorphWeights;
		if (!BindMaterial(sceneData, drawParameters, batch.materialSlot))
		{
			SPDLOG_LOGGER_WARN(
			    g_gbufferMeshBatchDrawerLogger,
			    "GBuffer: {} batch {} material slot {} has no valid texture binding set; skipped.",
			    ToMeshKindLabel(batch.meshKind),
			    batchIndex,
			    batch.materialSlot);
			return false;
		}

		const bool useTwoSidedPipeline = batch.materialSlot < sceneData.materials.size() &&
		                                 sceneData.materials[batch.materialSlot].doubleSided && runtime.TwoSidedPipelineState != nullptr;
		RasterPassPipelineRuntime batchRuntime{
		    runtime.BindingLayout,
		    useTwoSidedPipeline ? *runtime.TwoSidedPipelineState : runtime.PipelineState,
		    runtime.WireframePipelineState,
		    runtime.TwoSidedPipelineState};

		PassBindingOverrides overrides;
		overrides.SetDescriptorTable("SkinInfluences", gpuMesh.GetSkinInfluencesShaderResourceView());
		overrides.SetDescriptorTable("MorphTargetDeltas", gpuMesh.GetMorphTargetDeltasShaderResourceView());
		const bool bound = PassUtilities::BindAvailableRasterPassWithRuntime(
		    resources,
		    cmd,
		    &renderHardwareInterface,
		    batchRuntime,
		    drawParameters.GetPassParameterSet(),
		    &overrides,
		    GBufferPass::PassName,
		    false,
		    viewModeIndex);
		if (!bound)
		{
			SPDLOG_LOGGER_WARN(
			    g_gbufferMeshBatchDrawerLogger,
			    "GBuffer: shader binding layout rejected {} batch {}; skipped.",
			    ToMeshKindLabel(batch.meshKind),
			    batchIndex);
			assert(bound);
			return false;
		}

		cmd.DrawIndexedInstanced(gpuMesh.GetIndexCount(), batch.instanceCount, 0, 0, 0);
		return true;
	}

	static void DrawStaticBatch(
	    const FrameGraphResourceCommands& resources,
	    RenderCommandContext& cmd,
	    const FrameContext& frame,
	    const GBufferPass::Parameters& passParameters,
	    RenderHardwareInterface& renderHardwareInterface,
	    const RasterPassPipelineRuntime& runtime,
	    const GBufferPass::DrawParameterMetadata& drawParameterMetadata,
	    const GPUMesh& gpuMesh,
	    const MeshInstanceBatch& batch,
	    std::size_t batchIndex,
	    std::uint32_t viewModeIndex)
	{
		(void) DrawBatch(
		    resources,
		    cmd,
		    frame,
		    passParameters,
		    renderHardwareInterface,
		    runtime,
		    drawParameterMetadata,
		    gpuMesh,
		    batch,
		    batchIndex,
		    viewModeIndex);
	}

	static void DrawSkinnedBatch(
	    const FrameGraphResourceCommands& resources,
	    RenderCommandContext& cmd,
	    const FrameContext& frame,
	    const GBufferPass::Parameters& passParameters,
	    RenderHardwareInterface& renderHardwareInterface,
	    const RasterPassPipelineRuntime& runtime,
	    const GBufferPass::DrawParameterMetadata& drawParameterMetadata,
	    const GPUMesh& gpuMesh,
	    const MeshInstanceBatch& batch,
	    std::size_t batchIndex,
	    std::uint32_t viewModeIndex)
	{
		if (!frame.sceneGpuData->Geometry.HasSkinning())
		{
			SPDLOG_LOGGER_WARN(
			    g_gbufferMeshBatchDrawerLogger,
			    "GBuffer: skinned batch {} skipped because the frame skinning buffer is unavailable.",
			    batchIndex);
			return;
		}

		if (!HasValidSkinnedInstanceRange(frame.sceneData, batch))
		{
			SPDLOG_LOGGER_WARN(
			    g_gbufferMeshBatchDrawerLogger,
			    "GBuffer: skinned batch {} has an invalid joint palette binding; skipped.",
			    batchIndex);
			return;
		}

		(void) DrawBatch(
		    resources,
		    cmd,
		    frame,
		    passParameters,
		    renderHardwareInterface,
		    runtime,
		    drawParameterMetadata,
		    gpuMesh,
		    batch,
		    batchIndex,
		    viewModeIndex);
	}
};

void GBufferMeshBatchDrawer::DrawOpaqueMeshes(
    const FrameGraphResourceCommands& resources,
    RenderCommandContext& cmd,
    const FrameContext& frame,
    const GBufferPass::Parameters& parameters,
    const PassRuntimeServices& passRuntimeServices,
    const RasterPassPipelineRuntime& runtime,
    const GBufferPass::DrawParameterMetadata& drawParameterMetadata)
{
	RenderHardwareInterface& renderHardwareInterface = passRuntimeServices.HardwareInterface;
	const RenderSceneData& sceneData = frame.sceneData;
	const std::uint32_t viewModeIndex = passRuntimeServices.PerFrame.ViewModeIndex;
	if (passRuntimeServices.Meshes == nullptr)
	{
		return;
	}

	if (!frame.sceneGpuData->Geometry.HasMeshInstances())
	{
		if (!sceneData.meshInstanceBatches.empty())
		{
			SPDLOG_LOGGER_WARN(
			    g_gbufferMeshBatchDrawerLogger,
			    "GBuffer: {} instance batches skipped because the frame instance buffer is unavailable.",
			    sceneData.meshInstanceBatches.size());
		}
		return;
	}

	for (std::size_t batchIndex = 0; batchIndex < sceneData.meshInstanceBatches.size(); ++batchIndex)
	{
		const MeshInstanceBatch& batch = sceneData.meshInstanceBatches[batchIndex];
		const GPUMesh* gpuMesh =
		    GBufferMeshBatchDrawerOperations::ResolveValidBatch(
		        sceneData,
		        batch,
		        batchIndex,
		        *passRuntimeServices.Meshes);
		if (gpuMesh == nullptr)
		{
			continue;
		}

		if (batch.meshKind == RenderMeshKind::Skeletal)
		{
			GBufferMeshBatchDrawerOperations::DrawSkinnedBatch(
			    resources,
			    cmd,
			    frame,
			    parameters,
			    renderHardwareInterface,
			    runtime,
			    drawParameterMetadata,
			    *gpuMesh,
			    batch,
			    batchIndex,
			    viewModeIndex);
		}
		else
		{
			GBufferMeshBatchDrawerOperations::DrawStaticBatch(
			    resources,
			    cmd,
			    frame,
			    parameters,
			    renderHardwareInterface,
			    runtime,
			    drawParameterMetadata,
			    *gpuMesh,
			    batch,
			    batchIndex,
			    viewModeIndex);
		}
	}
}
