#pragma once

#include "Renderer/Public/FrameGraph/TextureHandle.h"
#include "Renderer/Public/Passes/ShaderSourceDefinition.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterFields.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "Renderer/Public/ShaderParameters/TypedPassParameterInstance.h"

#include "D3D12/Resources/D3D12ConstantBufferData.h"

#include <d3d12.h>

class D3D12ConstantBufferManager;
class CommandContext;
class FrameGraph;
class PassParameterLayout;
struct RenderGraphPassContext;
struct RenderPassContext;
struct RenderSceneData;
struct ShadowOpaquePassRuntime;
struct RenderViewContext;

struct ShadowOpaquePassParameters
{
	ShaderRenderTarget ShadowColor;
	ShaderDepthTarget ShadowDepth;
	ShaderUniform<PerFrameConstantBufferData> PerFrame;
	ShaderUniform<PerViewConstantBufferData> PerView;

	static void Describe(ShaderParameterStructBuilder<ShadowOpaquePassParameters>& builder)
	{
		builder.RenderTarget("ShadowColor", &ShadowOpaquePassParameters::ShadowColor, ShaderStageVisibility::AllGraphics);
		builder.DepthTarget("ShadowDepth", &ShadowOpaquePassParameters::ShadowDepth, ShaderStageVisibility::AllGraphics);
		builder.Uniform("PerFrame", &ShadowOpaquePassParameters::PerFrame, ShaderStageVisibility::AllGraphics);
		builder.Uniform("PerView", &ShadowOpaquePassParameters::PerView, ShaderStageVisibility::AllGraphics);
	}
};

class ShadowOpaquePass final
{
  public:
	static constexpr const char* PassName = "ShadowOpaque";
	using Parameters = ShadowOpaquePassParameters;

	using ParameterMetadata = ShaderParameterStructMetadata<Parameters>;
	using ParameterInstance = TypedPassParameterInstance<Parameters>;

	static const ParameterMetadata& GetParameterMetadata() noexcept;
	static ShaderSourceDefinition DescribeShadowViewVertexShader() noexcept;
	static ShaderSourceDefinition DescribeShadowViewPixelShader() noexcept;
	static void Execute(RenderGraphPassContext& context, ParameterInstance& parameters, std::size_t lightIndex);

  private:
	static void PrepareTargets(RenderGraphPassContext& context, const Parameters& parameters);
	static void PreparePassParameters(
	    ParameterInstance& parameters,
	    const RenderViewContext& viewContext,
	    const RenderPassContext& renderPassContext);
	static void ConfigurePipeline(CommandContext& cmd, const RenderViewContext& viewContext);
	static void BindPassResources(
	    const FrameGraph& frameGraph,
	    CommandContext& cmd,
	    const ParameterInstance& parameters,
	    const ShadowOpaquePassRuntime& runtime,
	    const RenderPassContext& renderPassContext,
	    D3D12_GPU_VIRTUAL_ADDRESS perViewGpuAddress);
	static void DrawMeshes(
	    const FrameGraph& frameGraph,
	    CommandContext& cmd,
	    const RenderSceneData& sceneData,
	    const ShadowOpaquePassRuntime& runtime,
	    const RenderPassContext& renderPassContext);
};