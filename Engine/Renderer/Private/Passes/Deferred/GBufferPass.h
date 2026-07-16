#pragma once

#include "Renderer/Public/ShaderParameters/ShaderParameterFields.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "Renderer/Public/ShaderParameters/TypedPassParameterInstance.h"
#include "Frame/Targets/FrameRenderTargets.h"

#include "ShaderData/RenderConstantBufferData.h"

#include <cstddef>

class RenderCommandContext;
struct RasterPassPipelineRuntime;
struct RenderPassDefinition;
struct PassExecutionContext;
struct RenderSceneData;
struct MeshInstanceBatch;
struct PassRuntimeServices;
struct RenderViewData;
struct FrameContext;
class FrameGraphResourceCommands;

struct GBufferPassParameters
{
	ShaderRenderTarget BaseColor;
	ShaderRenderTarget Normal;
	ShaderRenderTarget Material;
	ShaderRenderTarget Emissive;
	ShaderRenderTarget Subsurface;
	ShaderRenderTarget MotionVector;
	ShaderDepthTarget DeviceZ;
	ShaderUniform<PerFrameConstantBufferData> PerFrame;
	ShaderUniform<PerViewConstantBufferData> PerView;
	ShaderUniform<PerTemporalConstantBufferData> PerTemporal;
	ShaderSamplerSet SamplerAniso16xWrap;
	ShaderBuffer<MeshInstanceData> MeshInstances;
	ShaderBuffer<JointMatrixData> JointMatrices;
	ShaderBuffer<JointMatrixData> PreviousJointMatrices;

	static void Describe(ShaderParameterStructBuilder<GBufferPassParameters>& builder)
	{
		builder.RenderTarget("BaseColor", &GBufferPassParameters::BaseColor, ShaderStageVisibility::AllGraphics);
		builder.RenderTarget("Normal", &GBufferPassParameters::Normal, ShaderStageVisibility::AllGraphics);
		builder.RenderTarget("Material", &GBufferPassParameters::Material, ShaderStageVisibility::AllGraphics);
		builder.RenderTarget("Emissive", &GBufferPassParameters::Emissive, ShaderStageVisibility::AllGraphics);
		builder.RenderTarget("Subsurface", &GBufferPassParameters::Subsurface, ShaderStageVisibility::AllGraphics);
		builder.RenderTarget("MotionVector", &GBufferPassParameters::MotionVector, ShaderStageVisibility::AllGraphics);
		builder.DepthTarget("DeviceZ", &GBufferPassParameters::DeviceZ, ShaderStageVisibility::AllGraphics);
		builder.Uniform("PerFrame", &GBufferPassParameters::PerFrame, ShaderStageVisibility::Pixel);
		builder.Uniform("PerView", &GBufferPassParameters::PerView, ShaderStageVisibility::Vertex);
		builder.Uniform("PerTemporal", &GBufferPassParameters::PerTemporal, ShaderStageVisibility::Vertex | ShaderStageVisibility::Pixel);
		builder.Sampler("SamplerAniso16xWrap", &GBufferPassParameters::SamplerAniso16xWrap, ShaderStageVisibility::Pixel);
		builder.ReadBuffer("MeshInstances", &GBufferPassParameters::MeshInstances, ShaderStageVisibility::Vertex);
		builder.ReadBuffer("JointMatrices", &GBufferPassParameters::JointMatrices, ShaderStageVisibility::Vertex);
		builder.ReadBuffer("PreviousJointMatrices", &GBufferPassParameters::PreviousJointMatrices, ShaderStageVisibility::Vertex);
	}
};

struct GBufferDrawParameters
{
	ShaderUniform<MeshInstanceDrawConstantBufferData> MeshInstanceDraw;
	ShaderBuffer<MeshInstanceData> MeshInstances;
	ShaderBuffer<VertexSkinInfluenceData> SkinInfluences;
	ShaderBuffer<JointMatrixData> JointMatrices;
	ShaderBuffer<JointMatrixData> PreviousJointMatrices;
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
		builder.Uniform("MeshInstanceDraw", &GBufferDrawParameters::MeshInstanceDraw, ShaderStageVisibility::Vertex);
		builder.ReadBuffer("MeshInstances", &GBufferDrawParameters::MeshInstances, ShaderStageVisibility::Vertex);
		builder.ReadBuffer("SkinInfluences", &GBufferDrawParameters::SkinInfluences, ShaderStageVisibility::Vertex);
		builder.ReadBuffer("JointMatrices", &GBufferDrawParameters::JointMatrices, ShaderStageVisibility::Vertex);
		builder.ReadBuffer("PreviousJointMatrices", &GBufferDrawParameters::PreviousJointMatrices, ShaderStageVisibility::Vertex);
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
	using PipelineRuntime = RasterPassPipelineRuntime;

	explicit GBufferPass(const RasterPassPipelineRuntime& runtime) noexcept;

	static const ParameterMetadata& GetParameterMetadata() noexcept;
	static const DrawParameterMetadata& GetDrawParameterMetadata() noexcept;
	static const RenderPassDefinition& GetDefinition() noexcept;
	void Execute(PassExecutionContext& context, ParameterInstance& parameters) const;


  private:
	void SetParameters(ParameterInstance& parameters, const RenderViewData& viewData, const PassRuntimeServices& passRuntimeServices) const;
	void PrepareTargets(PassExecutionContext& context, const Parameters& parameters) const;
	void ConfigurePipeline(RenderCommandContext& cmd, const RenderViewData& viewData) const;
	void BindPassResources(
	    const FrameGraphResourceCommands& resources,
	    RenderCommandContext& cmd,
	    const ParameterInstance& parameters,
	    const PassRuntimeServices& passRuntimeServices) const;
	void DrawOpaqueMeshes(
	    const FrameGraphResourceCommands& resources,
	    RenderCommandContext& cmd,
	    const FrameContext& frame,
	    const Parameters& parameters,
	    const PassRuntimeServices& passRuntimeServices) const;

	const RasterPassPipelineRuntime& m_runtime;
};
