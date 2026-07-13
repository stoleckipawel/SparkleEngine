#pragma once

#include "Frame/Targets/FrameRenderTargets.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterFields.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "Renderer/Public/ShaderParameters/TypedPassParameterInstance.h"

#include "ShaderData/RenderConstantBufferData.h"
#include "ShaderData/RenderViewLightingData.h"

#include <cstdint>

class FrameGraphBuilder;
struct RenderPassDefinition;
struct ComputePassPipelineRuntime;
struct PassExecutionContext;
struct PassRuntimeServices;
struct FrameContext;
struct RenderViewData;
struct DirectShadowSignalResources;

struct DirectLightingPassParameters
{
	ShaderRWTexture2D<void> DirectDiffuse;
	ShaderRWTexture2D<void> DirectSpecular;
	ShaderRWTexture2D<void> DirectSubsurface;
	ShaderTexture2D<void> ShadowVisibilitySignal;
	ShaderTexture2D<void> CurrentReservoirSample;
	ShaderTexture2D<void> CurrentReservoirWeight;
	ShaderTexture2D<void> GBufferBaseColor;
	ShaderTexture2D<void> GBufferNormal;
	ShaderTexture2D<void> GBufferMaterial;
	ShaderTexture2D<void> GBufferSubsurface;
	ShaderTexture2D<void> SceneDepth;
	ShaderUniform<PerFrameConstantBufferData> PerFrame;
	ShaderUniform<PerViewConstantBufferData> PerView;
	ShaderUniform<PerTemporalConstantBufferData> PerTemporal;
	ShaderUniform<ViewLightingData> ViewLighting;
	ShaderBuffer<void> DirectionalLights;
	ShaderBuffer<void> PointLights;
	ShaderBuffer<void> SpotLights;
	ShaderBuffer<void> RectLights;

	static void Describe(ShaderParameterStructBuilder<DirectLightingPassParameters>& builder)
	{
		builder.RWTexture("DirectDiffuse", &DirectLightingPassParameters::DirectDiffuse, ShaderStageVisibility::Compute);
		builder.RWTexture("DirectSpecular", &DirectLightingPassParameters::DirectSpecular, ShaderStageVisibility::Compute);
		builder.RWTexture("DirectSubsurface", &DirectLightingPassParameters::DirectSubsurface, ShaderStageVisibility::Compute);
		builder.ReadTexture(
		    "ShadowVisibilitySignal",
		    &DirectLightingPassParameters::ShadowVisibilitySignal,
		    ShaderStageVisibility::Compute);
		builder.ReadTexture(
		    "CurrentReservoirSample",
		    &DirectLightingPassParameters::CurrentReservoirSample,
		    ShaderStageVisibility::Compute);
		builder.ReadTexture(
		    "CurrentReservoirWeight",
		    &DirectLightingPassParameters::CurrentReservoirWeight,
		    ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferBaseColor", &DirectLightingPassParameters::GBufferBaseColor, ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferNormal", &DirectLightingPassParameters::GBufferNormal, ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferMaterial", &DirectLightingPassParameters::GBufferMaterial, ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferSubsurface", &DirectLightingPassParameters::GBufferSubsurface, ShaderStageVisibility::Compute);
		builder.ReadTexture("SceneDepth", &DirectLightingPassParameters::SceneDepth, ShaderStageVisibility::Compute);
		builder.Uniform("PerFrame", &DirectLightingPassParameters::PerFrame, ShaderStageVisibility::Compute);
		builder.Uniform("PerView", &DirectLightingPassParameters::PerView, ShaderStageVisibility::Compute);
		builder.Uniform("PerTemporal", &DirectLightingPassParameters::PerTemporal, ShaderStageVisibility::Compute);
		builder.Uniform("ViewLighting", &DirectLightingPassParameters::ViewLighting, ShaderStageVisibility::Compute);
		builder.ReadBuffer("DirectionalLights", &DirectLightingPassParameters::DirectionalLights, ShaderStageVisibility::Compute);
		builder.ReadBuffer("PointLights", &DirectLightingPassParameters::PointLights, ShaderStageVisibility::Compute);
		builder.ReadBuffer("SpotLights", &DirectLightingPassParameters::SpotLights, ShaderStageVisibility::Compute);
		builder.ReadBuffer("RectLights", &DirectLightingPassParameters::RectLights, ShaderStageVisibility::Compute);
	}
};

class DirectLightingPass final
{
  public:
	static constexpr const char* PassName = "DirectLighting";
	static constexpr std::uint32_t ThreadGroupSizeX = 8;
	static constexpr std::uint32_t ThreadGroupSizeY = 8;
	using Parameters = DirectLightingPassParameters;
	using ParameterMetadata = ShaderParameterStructMetadata<Parameters>;
	using ParameterInstance = TypedPassParameterInstance<Parameters>;
	using PipelineRuntime = ComputePassPipelineRuntime;

	explicit DirectLightingPass(const ComputePassPipelineRuntime& runtime) noexcept;

	static const ParameterMetadata& GetParameterMetadata() noexcept;
	static const RenderPassDefinition& GetDefinition() noexcept;
	static void DeclareResources(
	    FrameGraphBuilder& builder,
	    const LightingRenderTargets& lighting,
	    FrameGraphTextureHandle sceneDepth,
	    const GBufferRenderTargets& gbuffer,
	    const DirectShadowSignalResources& shadowSignals,
	    FrameGraphBufferHandle directionalLights,
	    FrameGraphBufferHandle pointLights,
	    FrameGraphBufferHandle spotLights,
	    FrameGraphBufferHandle rectLights,
	    ParameterInstance& parameters);
	void Execute(PassExecutionContext& context, ParameterInstance& parameters) const;

  private:
	void SetParameters(
	    ParameterInstance& parameters,
	    const FrameContext& frame,
	    const RenderViewData& viewData,
	    const PassRuntimeServices& passRuntimeServices) const;

	const ComputePassPipelineRuntime& m_runtime;
};
