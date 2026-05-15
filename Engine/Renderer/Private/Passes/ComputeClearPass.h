#pragma once

#include "Renderer/Public/FrameGraph/TextureHandle.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterFields.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "Renderer/Public/ShaderParameters/TypedPassParameterInstance.h"
#include "RHI/Public/Shaders/CookedShaderPackageUtils.h"

#include <cstdint>

class RenderCommandContext;
class FrameGraph;
class PassParameterLayout;
struct RenderGraphPassContext;

struct ComputeClearPassParameters
{
	ShaderRWTexture2D<void> Output;

	static void Describe(ShaderParameterStructBuilder<ComputeClearPassParameters>& builder)
	{
		builder.RWTexture("Output", &ComputeClearPassParameters::Output, ShaderStageVisibility::Compute);
	}
};

class ComputeClearPass final
{
  public:
	static constexpr const char* PassName = "ComputeClear";
	static constexpr std::uint32_t ThreadGroupSizeX = 8;
	static constexpr std::uint32_t ThreadGroupSizeY = 8;
	using Parameters = ComputeClearPassParameters;

	using ParameterMetadata = ShaderParameterStructMetadata<Parameters>;
	using ParameterInstance = TypedPassParameterInstance<Parameters>;

	static const ParameterMetadata& GetParameterMetadata() noexcept;
	static const PassParameterLayout& GetParameterLayout() noexcept;
	static ShaderPackageDefinition DescribeShaderPackage() noexcept;
	static void DeclareResources(FrameGraph& frameGraph, TextureHandle outputTexture, ParameterInstance& parameters);
	static void Execute(
	    RenderGraphPassContext& context,
	    const ParameterInstance& parameters,
	    std::uint32_t width,
	    std::uint32_t height) noexcept;
};