#pragma once

#include "Renderer/Public/FrameGraph/TextureHandle.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterFields.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "Renderer/Public/ShaderParameters/TypedPassParameterInstance.h"
#include "FrameGraph/Features/FrameGraphProducts.h"

#include "RHI/Public/Resources/RenderConstantBufferData.h"
#include "RHI/Public/Shaders/CookedShaderPackageUtils.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"

class RenderCommandContext;
struct RasterPassRuntime;
class FrameGraph;
struct RenderGraphPassContext;
struct RenderSceneData;
struct RenderPassContext;
struct RenderViewData;

struct GBufferPassParameters
{
	ShaderRenderTarget BaseColor;
	ShaderRenderTarget Normal;
	ShaderRenderTarget Material;
	ShaderRenderTarget Emissive;
	ShaderRenderTarget Subsurface;
	ShaderRenderTarget DeviceZ;
	ShaderDepthTarget MainDepth;
	ShaderUniform<PerViewConstantBufferData> PerView;
	ShaderSamplerSet SamplerAniso16xWrap;

	static void Describe(ShaderParameterStructBuilder<GBufferPassParameters>& builder)
	{
		builder.RenderTarget("BaseColor", &GBufferPassParameters::BaseColor, ShaderStageVisibility::AllGraphics);
		builder.RenderTarget("Normal", &GBufferPassParameters::Normal, ShaderStageVisibility::AllGraphics);
		builder.RenderTarget("Material", &GBufferPassParameters::Material, ShaderStageVisibility::AllGraphics);
		builder.RenderTarget("Emissive", &GBufferPassParameters::Emissive, ShaderStageVisibility::AllGraphics);
		builder.RenderTarget("Subsurface", &GBufferPassParameters::Subsurface, ShaderStageVisibility::AllGraphics);
		builder.RenderTarget("DeviceZ", &GBufferPassParameters::DeviceZ, ShaderStageVisibility::AllGraphics);
		builder.DepthTarget("MainDepth", &GBufferPassParameters::MainDepth, ShaderStageVisibility::AllGraphics);
		builder.Uniform("PerView", &GBufferPassParameters::PerView, ShaderStageVisibility::Vertex);
		builder.Sampler("SamplerAniso16xWrap", &GBufferPassParameters::SamplerAniso16xWrap, ShaderStageVisibility::Pixel);
	}
};

struct GBufferDrawParameters
{
	ShaderUniform<PerObjectVSConstantBufferData> PerObjectVS;
	ShaderUniform<PerObjectPSConstantBufferData> PerObjectPS;
	ShaderTexture2DSRV TextureBaseColor;
	ShaderTexture2DSRV TextureNormal;
	ShaderTexture2DSRV TextureRoughness;
	ShaderTexture2DSRV TextureMetallic;
	ShaderTexture2DSRV TextureOcclusion;
	ShaderTexture2DSRV TextureEmissive;
	ShaderTexture2DSRV TextureSubsurfaceColor;
	ShaderTexture2DSRV TextureSubsurfaceStrength;

	static void Describe(ShaderParameterStructBuilder<GBufferDrawParameters>& builder)
	{
		builder.Uniform("PerObjectVS", &GBufferDrawParameters::PerObjectVS, ShaderStageVisibility::Vertex);
		builder.Uniform("PerObjectPS", &GBufferDrawParameters::PerObjectPS, ShaderStageVisibility::Pixel);
		builder.ReadTexture("TextureBaseColor", &GBufferDrawParameters::TextureBaseColor, ShaderStageVisibility::Pixel);
		builder.ReadTexture("TextureNormal", &GBufferDrawParameters::TextureNormal, ShaderStageVisibility::Pixel);
		builder.ReadTexture("TextureRoughness", &GBufferDrawParameters::TextureRoughness, ShaderStageVisibility::Pixel);
		builder.ReadTexture("TextureMetallic", &GBufferDrawParameters::TextureMetallic, ShaderStageVisibility::Pixel);
		builder.ReadTexture("TextureOcclusion", &GBufferDrawParameters::TextureOcclusion, ShaderStageVisibility::Pixel);
		builder.ReadTexture("TextureEmissive", &GBufferDrawParameters::TextureEmissive, ShaderStageVisibility::Pixel);
		builder.ReadTexture("TextureSubsurfaceColor", &GBufferDrawParameters::TextureSubsurfaceColor, ShaderStageVisibility::Pixel);
		builder.ReadTexture("TextureSubsurfaceStrength", &GBufferDrawParameters::TextureSubsurfaceStrength, ShaderStageVisibility::Pixel);
	}
};

class GBufferPass final
{
  public:
	static constexpr const char* PassName = "GBuffer";
	using Parameters = GBufferPassParameters;
	using DrawParameters = GBufferDrawParameters;

	using ParameterMetadata = ShaderParameterStructMetadata<Parameters>;
	using DrawParameterMetadata = ShaderParameterStructMetadata<DrawParameters>;
	using ParameterInstance = TypedPassParameterInstance<Parameters>;
	using DrawParameterInstance = TypedPassParameterInstance<DrawParameters>;

	static const ParameterMetadata& GetParameterMetadata() noexcept;
	static const DrawParameterMetadata& GetDrawParameterMetadata() noexcept;
	static ShaderPackageDefinition DescribeGBufferShaderPackage() noexcept;
	static void DeclareResources(FrameGraph& frameGraph, const GBufferTargets& targets, ParameterInstance& parameters);
	static void SetParameters(ParameterInstance& parameters, const RenderViewData& viewData);
	static void Execute(RenderGraphPassContext& context, ParameterInstance& parameters);

  private:
	static void PrepareTargets(RenderGraphPassContext& context, const Parameters& parameters);
	static void ConfigurePipeline(RenderCommandContext& cmd, const RenderViewData& viewData);
	static void BindPassResources(
	    const FrameGraph& frameGraph,
	    RenderCommandContext& cmd,
	    const ParameterInstance& parameters,
	    const RasterPassRuntime& runtime,
	    const RenderPassContext& renderPassContext);
	static void DrawOpaqueMeshes(
	    const FrameGraph& frameGraph,
	    RenderCommandContext& cmd,
	    const RenderSceneData& sceneData,
	    const RasterPassRuntime& runtime,
	    const RenderPassContext& renderPassContext);
};
