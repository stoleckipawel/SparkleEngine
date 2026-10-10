#pragma once

#include "Passes/GBuffer/Raster/GBufferShaders.h"
#include "RHI/Public/Descriptors/RhiDescriptorHandles.h"
#include "RHI/Public/Pipeline/RhiPipelineDesc.h"
#include "ShaderData/PerObjectConstantBufferData.h"
#include "ShaderParameters/ShaderParameterStructBuilder.h"
#include "ShaderParameters/TypedPassParameterInstance.h"

#include <functional>
#include <cstdint>
#include <vector>

class RenderCommandContext;
class GpuMesh;
class RenderBindingLayout;
class RenderPipeline;
struct MaterialData;
struct MeshInstanceBatch;
class GpuMeshCache;
class RasterPassRenderState;
class RenderPassRuntimeCache;
struct GraphicsAttachmentSignature;
struct PassCommandContext;
struct PreparedRenderScene;
struct RenderView;
class FrameGraphResourceCommands;

struct GBufferShaderParameters final
{
	GBufferVS::Parameters Vertex;
	GBufferPS::Parameters Pixel;

	static void Describe(ShaderParameterStructBuilder<GBufferShaderParameters>& builder);
};

struct GBufferGraphParameters final
{
	ShaderRenderTarget BaseColor;
	ShaderRenderTarget WorldNormal;
	ShaderRenderTarget WorldTangent;
	ShaderRenderTarget Material;
	ShaderRenderTarget Emissive;
	ShaderRenderTarget Subsurface;
	ShaderRenderTarget MotionVector;
	ShaderDepthTarget DeviceZ;
	GBufferShaderParameters Shader;

	static void Describe(ShaderParameterStructBuilder<GBufferGraphParameters>& builder);
};

class GBufferMeshPass final
{
public:
	using Parameters = GBufferGraphParameters;

	using ParameterInstance = TypedPassParameterInstance<Parameters>;

	GBufferMeshPass(GpuMeshCache& gpuMeshCache, const PreparedRenderScene& scene, const RenderView& view) noexcept;
	GBufferMeshPass(const GBufferMeshPass&) = delete;
	GBufferMeshPass(GBufferMeshPass&&) noexcept = default;

	void MaterializePipelines(const RenderPassRuntimeCache& runtimeCache, const RasterPassRenderState& renderState, const GraphicsAttachmentSignature& attachments);
	void PrepareRasterPass(RenderCommandContext& commandContext) const;
	void Draw(PassCommandContext& context, ParameterInstance& parameters);

private:
	using DrawParameters = GBufferShaderParameters;

	using DrawParameterMetadata = ShaderParameterStructMetadata<DrawParameters>;
	using DrawParameterInstance = TypedPassParameterInstance<DrawParameters>;

	static const DrawParameterMetadata& GetDrawParameterMetadata() noexcept;

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

	static void BindMaterial(DrawParameterInstance& drawParameters, const PreparedDraw& draw);
	static const GpuMesh* ResolveBatch(const RenderView& view, const MeshInstanceBatch& batch, const GpuMeshCache& meshes) noexcept;
	static bool HasValidSkinning(const PreparedRenderScene& preparedScene, const RenderView& view, const MeshInstanceBatch& batch) noexcept;
	static void ConfigureDrawParameters(const Parameters& passParameters, std::uint32_t firstInstance, DrawParameterInstance& drawParameters);
	static RhiRasterizerState ResolveRasterizerState(const MaterialData& material, const GpuMesh& gpuMesh, bool wireframe) noexcept;
	static bool BindBatchPipeline(const FrameGraphResourceCommands& resources, RenderCommandContext& commandContext, DrawParameterInstance& drawParameters, const PreparedDraw& draw);
	static void DrawBatch(
	    const FrameGraphResourceCommands& resources,
	    RenderCommandContext& commandContext,
	    const Parameters& passParameters,
	    const DrawParameterMetadata& drawParameterMetadata,
	    const PreparedDraw& draw);

	const GpuMeshCache& m_gpuMeshCache;
	const PreparedRenderScene& m_scene;
	const RenderView& m_view;
	std::vector<PreparedDraw> m_preparedDraws;
};
