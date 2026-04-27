#pragma once

#include "Renderer/Public/FrameGraph/TextureHandle.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterFields.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "Renderer/Public/ShaderParameters/TypedPassParameterInstance.h"

#include "RHI/Public/Resources/RenderConstantBufferData.h"
#include "RHI/Public/Shaders/CookedShaderPackageUtils.h"
#include "RHI/Public/Interop/RenderHardwareInterface.h"

class CommandContext;
struct ForwardOpaquePassRuntime;
class FrameGraph;
class PassParameterLayout;
struct RenderGraphPassContext;
struct RenderSceneData;
struct RenderPassContext;
struct RenderViewContext;

struct ForwardOpaquePassParameters
{
	ShaderRenderTarget BackBuffer;
	ShaderDepthTarget MainDepth;
	ShaderTexture2D<void> ShadowMap0;
	ShaderTexture2D<void> ShadowMap1;
	ShaderTexture2D<void> ShadowMap2;
	ShaderTexture2D<void> ShadowMap3;
	ShaderUniform<PerFrameConstantBufferData> PerFrame;
	ShaderUniform<PerViewConstantBufferData> PerView;
	ShaderSamplerSet SamplerAniso16xWrap;
	ShaderSamplerSet SamplerLinearNoMipClamp;

	static void Describe(ShaderParameterStructBuilder<ForwardOpaquePassParameters>& builder)
	{
		builder.RenderTarget("BackBuffer", &ForwardOpaquePassParameters::BackBuffer, ShaderStageVisibility::AllGraphics);
		builder.DepthTarget("MainDepth", &ForwardOpaquePassParameters::MainDepth, ShaderStageVisibility::AllGraphics);
		builder.ReadTexture("ShadowMap0", &ForwardOpaquePassParameters::ShadowMap0, ShaderStageVisibility::Pixel);
		builder.ReadTexture("ShadowMap1", &ForwardOpaquePassParameters::ShadowMap1, ShaderStageVisibility::Pixel);
		builder.ReadTexture("ShadowMap2", &ForwardOpaquePassParameters::ShadowMap2, ShaderStageVisibility::Pixel);
		builder.ReadTexture("ShadowMap3", &ForwardOpaquePassParameters::ShadowMap3, ShaderStageVisibility::Pixel);
		builder.Uniform("PerFrame", &ForwardOpaquePassParameters::PerFrame, ShaderStageVisibility::AllGraphics);
		builder.Uniform("PerView", &ForwardOpaquePassParameters::PerView, ShaderStageVisibility::AllGraphics);
		builder.Sampler("SamplerAniso16xWrap", &ForwardOpaquePassParameters::SamplerAniso16xWrap, ShaderStageVisibility::Pixel);
		builder.Sampler("SamplerLinearNoMipClamp", &ForwardOpaquePassParameters::SamplerLinearNoMipClamp, ShaderStageVisibility::Pixel);
	}
};

struct ForwardOpaqueDrawParameters
{
	ShaderUniform<PerObjectVSConstantBufferData> PerObjectVS;
	ShaderUniform<PerObjectPSConstantBufferData> PerObjectPS;
	ShaderTexture2DSRV TextureBaseColor;
	ShaderTexture2DSRV TextureNormal;
	ShaderTexture2DSRV TextureMetallicRoughness;
	ShaderTexture2DSRV TextureOcclusion;
	ShaderTexture2DSRV TextureEmissive;

	static void Describe(ShaderParameterStructBuilder<ForwardOpaqueDrawParameters>& builder)
	{
		builder.Uniform("PerObjectVS", &ForwardOpaqueDrawParameters::PerObjectVS, ShaderStageVisibility::Vertex);
		builder.Uniform("PerObjectPS", &ForwardOpaqueDrawParameters::PerObjectPS, ShaderStageVisibility::Pixel);
		builder.ReadTexture("TextureBaseColor", &ForwardOpaqueDrawParameters::TextureBaseColor, ShaderStageVisibility::Pixel);
		builder.ReadTexture("TextureNormal", &ForwardOpaqueDrawParameters::TextureNormal, ShaderStageVisibility::Pixel);
		builder.ReadTexture("TextureMetallicRoughness", &ForwardOpaqueDrawParameters::TextureMetallicRoughness, ShaderStageVisibility::Pixel);
		builder.ReadTexture("TextureOcclusion", &ForwardOpaqueDrawParameters::TextureOcclusion, ShaderStageVisibility::Pixel);
		builder.ReadTexture("TextureEmissive", &ForwardOpaqueDrawParameters::TextureEmissive, ShaderStageVisibility::Pixel);
	}
};

class ForwardOpaquePass final
{
  public:
	static constexpr const char* PassName = "ForwardOpaque";
	using Parameters = ForwardOpaquePassParameters;
	using DrawParameters = ForwardOpaqueDrawParameters;

	using ParameterMetadata = ShaderParameterStructMetadata<Parameters>;
	using DrawParameterMetadata = ShaderParameterStructMetadata<DrawParameters>;
	using ParameterInstance = TypedPassParameterInstance<Parameters>;
	using DrawParameterInstance = TypedPassParameterInstance<DrawParameters>;

	static const ParameterMetadata& GetParameterMetadata() noexcept;
	static const DrawParameterMetadata& GetDrawParameterMetadata() noexcept;
	static ShaderPackageDefinition DescribePrimaryViewShaderPackage() noexcept;
	static void Execute(RenderGraphPassContext& context, ParameterInstance& parameters);

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
	    const ForwardOpaquePassRuntime& runtime,
	    const RenderPassContext& renderPassContext,
	    RhiGpuVirtualAddress perViewGpuAddress);
	static void DrawOpaqueMeshes(
	    const FrameGraph& frameGraph,
	    CommandContext& cmd,
	    const RenderSceneData& sceneData,
	    const ForwardOpaquePassRuntime& runtime,
	    const RenderPassContext& renderPassContext);
};
