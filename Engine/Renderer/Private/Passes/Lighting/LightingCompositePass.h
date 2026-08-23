#pragma once

#include "Frame/Graph/RenderFrameGraphTargets.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterFields.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "Renderer/Public/ShaderParameters/TypedPassParameterInstance.h"

#include <cstdint>

struct RenderPassDefinition;
struct ComputePassPipelineRuntime;
struct PassCommandContext;

struct LightingCompositePassParameters
{
	ShaderRWTexture2D<void> SceneColor;
	ShaderTexture2D<void> DirectDiffuse;
	ShaderTexture2D<void> DirectSpecular;
	ShaderTexture2D<void> DirectSubsurface;
	ShaderTexture2D<void> IndirectDiffuse;
	ShaderTexture2D<void> IndirectSpecular;
	ShaderTexture2D<void> GBufferBaseColor;
	ShaderTexture2D<void> GBufferEmissive;

	static void Describe(ShaderParameterStructBuilder<LightingCompositePassParameters>& builder)
	{
		builder.RWTexture("SceneColor", &LightingCompositePassParameters::SceneColor, ShaderStageVisibility::Compute);
		builder.ReadTexture("DirectDiffuse", &LightingCompositePassParameters::DirectDiffuse, ShaderStageVisibility::Compute);
		builder.ReadTexture("DirectSpecular", &LightingCompositePassParameters::DirectSpecular, ShaderStageVisibility::Compute);
		builder.ReadTexture("DirectSubsurface", &LightingCompositePassParameters::DirectSubsurface, ShaderStageVisibility::Compute);
		builder.ReadTexture("IndirectDiffuse", &LightingCompositePassParameters::IndirectDiffuse, ShaderStageVisibility::Compute);
		builder.ReadTexture("IndirectSpecular", &LightingCompositePassParameters::IndirectSpecular, ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferBaseColor", &LightingCompositePassParameters::GBufferBaseColor, ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferEmissive", &LightingCompositePassParameters::GBufferEmissive, ShaderStageVisibility::Compute);
	}
};

class LightingCompositePass final
{
public:
	static constexpr const char* PassName = "LightingComposite";
	static constexpr std::uint32_t ThreadGroupSizeX = 8;
	static constexpr std::uint32_t ThreadGroupSizeY = 8;
	using Parameters = LightingCompositePassParameters;
	using ParameterMetadata = ShaderParameterStructMetadata<Parameters>;
	using ParameterInstance = TypedPassParameterInstance<Parameters>;
	using PipelineRuntime = ComputePassPipelineRuntime;

	explicit LightingCompositePass(const ComputePassPipelineRuntime& runtime) noexcept;

	static const ParameterMetadata& GetParameterMetadata() noexcept;
	static const RenderPassDefinition& GetDefinition() noexcept;
	void Execute(PassCommandContext& context, ParameterInstance& parameters, std::uint32_t outputWidth, std::uint32_t outputHeight) const;

private:
	const ComputePassPipelineRuntime& m_runtime;
};
