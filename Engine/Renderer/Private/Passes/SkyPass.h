#pragma once

#include "FrameGraph/Features/FrameGraphProducts.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterFields.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "Renderer/Public/ShaderParameters/TypedPassParameterInstance.h"

#include "RHI/Public/Resources/RenderConstantBufferData.h"
#include "RHI/Public/Shaders/CookedShaderPackageUtils.h"

#include <cstdint>

class FrameGraph;
struct RenderGraphPassContext;
struct RenderPassContext;
struct RenderViewContext;
struct SkyPassRuntime;

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

	static const ParameterMetadata& GetParameterMetadata() noexcept;
	static ShaderPackageDefinition DescribeShaderPackage() noexcept;
	static void DeclareResources(
		FrameGraph& frameGraph,
		const SceneTargets& sceneTargets,
		const GBufferTargets& gbuffer,
		ParameterInstance& parameters);
	static void SetParameters(
		ParameterInstance& parameters,
		const RenderViewContext& viewContext,
		const RenderPassContext& renderPassContext);
	static void Execute(RenderGraphPassContext& context, ParameterInstance& parameters);
};