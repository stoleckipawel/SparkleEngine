#pragma once

#include "Passes/GBuffer/Raster/GBufferShaders.h"
#include "RHI/Public/Resources/RhiResourceDesc.h"
#include "ShaderParameters/ShaderParameterStructBuilder.h"
#include "ShaderParameters/TypedPassParameterInstance.h"

#include <functional>
#include <memory>

class RenderCommandContext;
class GBufferMeshBatchDrawer;
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
	using DrawParameters = GBufferShaderParameters;

	using DrawParameterMetadata = ShaderParameterStructMetadata<DrawParameters>;
	using ParameterInstance = TypedPassParameterInstance<Parameters>;
	using DrawParameterInstance = TypedPassParameterInstance<DrawParameters>;

	GBufferMeshPass(GpuMeshCache& gpuMeshCache, const PreparedRenderScene& scene, const RenderView& view) noexcept;
	~GBufferMeshPass() noexcept;

	static const DrawParameterMetadata& GetDrawParameterMetadata() noexcept;
	void MaterializePipelines(
	    const RenderPassRuntimeCache& runtimeCache,
	    const RasterPassRenderState& renderState,
	    const GraphicsAttachmentSignature& attachments) const;
	void PrepareRasterPass(RenderCommandContext& commandContext) const;
	void Draw(PassCommandContext& context, ParameterInstance& parameters) const;

private:
	void DrawPreparedMeshes(
	    const FrameGraphResourceCommands& resources,
	    RenderCommandContext& commandContext,
	    const Parameters& parameters) const;

	std::shared_ptr<GBufferMeshBatchDrawer> m_meshBatchDrawer;
	std::reference_wrapper<const PreparedRenderScene> m_scene;
	std::reference_wrapper<const RenderView> m_view;
};
