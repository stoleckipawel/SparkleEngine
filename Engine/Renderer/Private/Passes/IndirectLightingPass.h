#pragma once

#include "FrameGraph/Features/FrameGraphProducts.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterFields.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "Renderer/Public/ShaderParameters/TypedPassParameterInstance.h"

#include "RHI/Public/Shaders/CookedShaderPackageUtils.h"

#include <cstdint>

class FrameGraph;
struct IndirectLightingPassRuntime;
struct RenderGraphPassContext;

struct IndirectLightingPassParameters
{
	ShaderRWTexture2D<void> IndirectDiffuse;
	ShaderRWTexture2D<void> IndirectSpecular;
	ShaderRWTexture2D<void> IndirectSubsurface;

	static void Describe(ShaderParameterStructBuilder<IndirectLightingPassParameters>& builder)
	{
		builder.RWTexture("IndirectDiffuse", &IndirectLightingPassParameters::IndirectDiffuse, ShaderStageVisibility::Compute);
		builder.RWTexture("IndirectSpecular", &IndirectLightingPassParameters::IndirectSpecular, ShaderStageVisibility::Compute);
		builder.RWTexture("IndirectSubsurface", &IndirectLightingPassParameters::IndirectSubsurface, ShaderStageVisibility::Compute);
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

	static const ParameterMetadata& GetParameterMetadata() noexcept;
	static ShaderPackageDefinition DescribeShaderPackage() noexcept;
	static void DeclareResources(
		FrameGraph& frameGraph,
		const LightingTargets& lighting,
		ParameterInstance& parameters);
	static void Execute(RenderGraphPassContext& context, ParameterInstance& parameters);
};