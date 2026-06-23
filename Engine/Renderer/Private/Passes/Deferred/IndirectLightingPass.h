#pragma once

#include "Frame/Targets/FrameRenderTargets.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterFields.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "Renderer/Public/ShaderParameters/TypedPassParameterInstance.h"

#include "Passes/Bindings/EnvironmentMapPassBinding.h"

#include <cstdint>

class FrameGraphBuilder;
struct RenderPassDefinition;
struct ComputePassPipelineRuntime;
struct PassExecutionContext;
struct PassRuntimeServices;

struct IndirectLightingPassParameters
{
	ShaderRWTexture2D<void> IndirectDiffuse;
	ShaderTexture2D<void> GBufferNormal;
	ShaderTexture2D<void> GBufferDeviceZ;
	ShaderTexture2DSRV SkyTexture;
	ShaderSamplerSet SamplerLinearNoMipClamp;

	static void Describe(ShaderParameterStructBuilder<IndirectLightingPassParameters>& builder)
	{
		builder.RWTexture("IndirectDiffuse", &IndirectLightingPassParameters::IndirectDiffuse, ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferNormal", &IndirectLightingPassParameters::GBufferNormal, ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferDeviceZ", &IndirectLightingPassParameters::GBufferDeviceZ, ShaderStageVisibility::Compute);
		builder.ReadTexture("SkyTexture", &IndirectLightingPassParameters::SkyTexture, ShaderStageVisibility::Compute);
		builder.Sampler("SamplerLinearNoMipClamp", &IndirectLightingPassParameters::SamplerLinearNoMipClamp, ShaderStageVisibility::Compute);
	}
};

class IndirectLightingPass final
{
  public:
	static constexpr const char* PassName = "IndirectLighting";
	static constexpr std::uint32_t ThreadGroupSizeX = 8;
	static constexpr std::uint32_t ThreadGroupSizeY = 8;
	using Parameters = IndirectLightingPassParameters;
	using ParameterMetadata = ShaderParameterStructMetadata<Parameters>;
	using ParameterInstance = TypedPassParameterInstance<Parameters>;
	using PipelineRuntime = ComputePassPipelineRuntime;

	explicit IndirectLightingPass(const ComputePassPipelineRuntime& runtime) noexcept;

	static const ParameterMetadata& GetParameterMetadata() noexcept;
	static const RenderPassDefinition& GetDefinition() noexcept;
	static void DeclareResources(
	    FrameGraphBuilder& builder,
	    const LightingRenderTargets& lighting,
	    const GBufferRenderTargets& gbuffer,
	    ParameterInstance& parameters);
	void Execute(PassExecutionContext& context, ParameterInstance& parameters) const;

  private:
	void SetParameters(ParameterInstance& parameters, const PassRuntimeServices& passRuntimeServices) const;

	const ComputePassPipelineRuntime& m_runtime;
	mutable EnvironmentMapPassBinding m_environmentMapBinding;
};
