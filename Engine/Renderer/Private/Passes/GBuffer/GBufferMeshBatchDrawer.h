#pragma once

#include "Passes/GBuffer/GBufferMeshPass.h"
#include "RHI/Public/Descriptors/RhiDescriptorHandles.h"
#include "RHI/Public/Pipeline/RhiPipelineDesc.h"
#include "ShaderData/PerObjectConstantBufferData.h"

#include <cstdint>
#include <functional>
#include <vector>

class FrameGraphResourceCommands;
class GpuMesh;
class GpuMeshCache;
class RenderCommandContext;
class RasterPassRenderState;
class RenderPassRuntimeCache;
struct GraphicsAttachmentSignature;
struct MaterialData;
struct MeshInstanceBatch;
struct PreparedRenderScene;
struct RenderView;

class GBufferMeshBatchDrawer final
{
public:
	explicit GBufferMeshBatchDrawer(const GpuMeshCache& gpuMeshCache) noexcept :
	    m_gpuMeshCache(gpuMeshCache)
	{
	}
	~GBufferMeshBatchDrawer() noexcept;

	void DrawPreparedMeshes(
	    const FrameGraphResourceCommands& resources,
	    RenderCommandContext& commandContext,
	    const GBufferMeshPass::Parameters& parameters,
	    const GBufferMeshPass::DrawParameterMetadata& drawParameterMetadata);
	void PrepareDrawsAndMaterializePipelines(
	    const RenderPassRuntimeCache& runtimeCache,
	    const PreparedRenderScene& preparedScene,
	    const RenderView& view,
	    const RasterPassRenderState& renderState,
	    const GraphicsAttachmentSignature& attachments,
	    bool wireframe);

private:
	struct PreparedDraw final
	{
		std::reference_wrapper<const GpuMesh> Mesh;
		std::uint32_t FirstInstance;
		std::uint32_t InstanceCount;
		PerObjectPSConstantBufferData MaterialParameters;
		RhiDescriptorTableHandle MaterialTextures;
		std::reference_wrapper<const RenderBindingLayout> BindingLayout;
		std::reference_wrapper<const RenderPipeline> Pipeline;
	};

	static void BindMaterial(GBufferMeshPass::DrawParameterInstance& drawParameters, const PreparedDraw& draw);
	static const GpuMesh* ResolveBatch(const RenderView& view, const MeshInstanceBatch& batch, const GpuMeshCache& meshes) noexcept;
	static bool HasValidSkinning(const PreparedRenderScene& preparedScene, const RenderView& view, const MeshInstanceBatch& batch) noexcept;
	static void ConfigureDrawParameters(
	    const GBufferMeshPass::Parameters& passParameters,
	    std::uint32_t firstInstance,
	    GBufferMeshPass::DrawParameterInstance& drawParameters);
	static RhiRasterizerState ResolveRasterizerState(const MaterialData& material, const GpuMesh& gpuMesh, bool wireframe) noexcept;
	static bool BindBatchPipeline(
	    const FrameGraphResourceCommands& resources,
	    RenderCommandContext& commandContext,
	    GBufferMeshPass::DrawParameterInstance& drawParameters,
	    const PreparedDraw& draw);
	static void DrawBatch(
	    const FrameGraphResourceCommands& resources,
	    RenderCommandContext& commandContext,
	    const GBufferMeshPass::Parameters& passParameters,
	    const GBufferMeshPass::DrawParameterMetadata& drawParameterMetadata,
	    const PreparedDraw& draw);

	const GpuMeshCache& m_gpuMeshCache;
	std::vector<PreparedDraw> m_preparedDraws;
};
