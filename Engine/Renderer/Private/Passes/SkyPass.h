#pragma once

#include "Frame/Targets/FrameRenderTargets.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterFields.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "Renderer/Public/ShaderParameters/TypedPassParameterInstance.h"

#include "RHI/Public/Resources/RenderConstantBufferData.h"
#include "RHI/Public/Shaders/CookedShaderPackageUtils.h"

#include <cstdint>

class FrameGraphBuilder;
struct ComputePassPipelineRuntime;
struct PassExecutionContext;
struct PassRuntimeServices;
struct RenderViewData;

struct SkyPassParameters
{
	ShaderRWTexture2D<void> SceneColor;
	ShaderTexture2D<void> GBufferDeviceZ;
	ShaderSamplerSet SamplerLinearClamp;
	ShaderUniform<PerFrameConstantBufferData> PerFrame;
	ShaderUniform<PerViewConstantBufferData> PerView;

	static void Describe(ShaderParameterStructBuilder<SkyPassParameters>& builder)
	{
		builder.RWTexture("SceneColor", &SkyPassParameters::SceneColor, ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferDeviceZ", &SkyPassParameters::GBufferDeviceZ, ShaderStageVisibility::Compute);
		builder.Sampler("SamplerLinearClamp", &SkyPassParameters::SamplerLinearClamp, ShaderStageVisibility::Compute);
		builder.Uniform("PerFrame", &SkyPassParameters::PerFrame, ShaderStageVisibility::Compute);
		builder.Uniform("PerView", &SkyPassParameters::PerView, ShaderStageVisibility::Compute);
	}
};

class SkyPass final
{
  public:
	static constexpr const char* PassName = "Sky";
	static constexpr std::uint32_t ThreadGroupSizeX = 8;
	static constexpr std::uint32_t ThreadGroupSizeY = 8;
	using Parameters = SkyPassParameters;
	using ParameterMetadata = ShaderParameterStructMetadata<Parameters>;
	using ParameterInstance = TypedPassParameterInstance<Parameters>;

	explicit SkyPass(const ComputePassPipelineRuntime& runtime) noexcept;

	static const ParameterMetadata& GetParameterMetadata() noexcept;
	static ShaderPackageDefinition DescribeShaderPackage() noexcept;
	static void DeclareResources(
	    FrameGraphBuilder& builder,
	    const SceneRenderTargets& sceneTargets,
	    const GBufferRenderTargets& gbuffer,
	    ParameterInstance& parameters);
	void Execute(PassExecutionContext& context, ParameterInstance& parameters) const;

  private:
	void SetParameters(
	    ParameterInstance& parameters,
	    const RenderViewData& viewData,
	    const PassRuntimeServices& passRuntimeServices) const;

	const ComputePassPipelineRuntime& m_runtime;
};
