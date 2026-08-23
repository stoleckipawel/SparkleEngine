#pragma once

#include "Passes/GBuffer/GBufferShaders.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "Renderer/Public/ShaderParameters/TypedPassParameterInstance.h"

#include <functional>
#include <memory>
#include <optional>

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

struct GBufferMeshPassInput final
{
	// These required values are borrowed only between frame-graph setup and the synchronous recording call.
	std::optional<std::reference_wrapper<const PreparedRenderScene>> PreparedScene;
	std::optional<std::reference_wrapper<const RenderView>> View;
	bool Wireframe = false;
};

struct GBufferShaderParameters final
{
	GBufferVS::Parameters Vertex;
	GBufferPS::Parameters Pixel;

	static void Describe(ShaderParameterStructBuilder<GBufferShaderParameters>& builder);
};

struct GBufferGraphParameters final
{
	ShaderRenderTarget BaseColor;
	ShaderRenderTarget Normal;
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

	GBufferMeshPass(GpuMeshCache& gpuMeshCache, const std::shared_ptr<GBufferMeshPassInput>& frameInput) noexcept;
	~GBufferMeshPass() noexcept;

	static const DrawParameterMetadata& GetDrawParameterMetadata() noexcept;
	void MaterializePipelines(
	    const RenderPassRuntimeCache& runtimeCache,
	    const RasterPassRenderState& renderState,
	    const GraphicsAttachmentSignature& attachments) const;
	void PrepareRasterPass(RenderCommandContext& commandContext) const;
	void Draw(
	    PassCommandContext& context,
	    ParameterInstance& parameters,
	    const RenderPassRuntimeCache& runtimeCache,
	    const RasterPassRenderState& renderState,
	    const GraphicsAttachmentSignature& attachments) const;

private:
	void DrawOpaqueMeshes(
	    const FrameGraphResourceCommands& resources,
	    RenderCommandContext& commandContext,
	    const PreparedRenderScene& preparedScene,
	    const RenderView& view,
	    const Parameters& parameters,
	    const RenderPassRuntimeCache& runtimeCache,
	    const RasterPassRenderState& renderState,
	    const GraphicsAttachmentSignature& attachments,
	    bool wireframe) const;

	std::shared_ptr<const GBufferMeshBatchDrawer> m_meshBatchDrawer;
	std::shared_ptr<GBufferMeshPassInput> m_frameInput;
};
