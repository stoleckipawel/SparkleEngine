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

struct VisualizeBuffersPassParameters
{
	ShaderRWTexture2D<void> SceneColor;
	ShaderTexture2D<void> DirectDiffuse;
	ShaderTexture2D<void> DirectSpecular;
	ShaderTexture2D<void> DirectSubsurface;
	ShaderTexture2D<void> IndirectDiffuse;
	ShaderTexture2D<void> IndirectSpecular;
	ShaderTexture2D<void> IndirectSubsurface;
	ShaderTexture2D<void> GBufferBaseColor;
	ShaderTexture2D<void> GBufferNormal;
	ShaderTexture2D<void> GBufferMaterial;
	ShaderTexture2D<void> GBufferEmissive;
	ShaderTexture2D<void> GBufferSubsurface;
	ShaderUniform<PerFrameConstantBufferData> PerFrame;

	static void Describe(ShaderParameterStructBuilder<VisualizeBuffersPassParameters>& builder)
	{
		builder.RWTexture("SceneColor", &VisualizeBuffersPassParameters::SceneColor, ShaderStageVisibility::Compute);
		builder.ReadTexture("DirectDiffuse", &VisualizeBuffersPassParameters::DirectDiffuse, ShaderStageVisibility::Compute);
		builder.ReadTexture("DirectSpecular", &VisualizeBuffersPassParameters::DirectSpecular, ShaderStageVisibility::Compute);
		builder.ReadTexture("DirectSubsurface", &VisualizeBuffersPassParameters::DirectSubsurface, ShaderStageVisibility::Compute);
		builder.ReadTexture("IndirectDiffuse", &VisualizeBuffersPassParameters::IndirectDiffuse, ShaderStageVisibility::Compute);
		builder.ReadTexture("IndirectSpecular", &VisualizeBuffersPassParameters::IndirectSpecular, ShaderStageVisibility::Compute);
		builder.ReadTexture("IndirectSubsurface", &VisualizeBuffersPassParameters::IndirectSubsurface, ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferBaseColor", &VisualizeBuffersPassParameters::GBufferBaseColor, ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferNormal", &VisualizeBuffersPassParameters::GBufferNormal, ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferMaterial", &VisualizeBuffersPassParameters::GBufferMaterial, ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferEmissive", &VisualizeBuffersPassParameters::GBufferEmissive, ShaderStageVisibility::Compute);
		builder.ReadTexture("GBufferSubsurface", &VisualizeBuffersPassParameters::GBufferSubsurface, ShaderStageVisibility::Compute);
		builder.Uniform("PerFrame", &VisualizeBuffersPassParameters::PerFrame, ShaderStageVisibility::Compute);
	}
};

class VisualizeBuffersPass final
{
  public:
	static constexpr const char* PassName = "VisualizeBuffers";
	static constexpr std::uint32_t ThreadGroupSizeX = 8;
	static constexpr std::uint32_t ThreadGroupSizeY = 8;
	using Parameters = VisualizeBuffersPassParameters;
	using ParameterMetadata = ShaderParameterStructMetadata<Parameters>;
	using ParameterInstance = TypedPassParameterInstance<Parameters>;

	explicit VisualizeBuffersPass(const ComputePassPipelineRuntime& runtime) noexcept;

	static const ParameterMetadata& GetParameterMetadata() noexcept;
	static ShaderPackageDefinition DescribeShaderPackage() noexcept;
	static void DeclareResources(
	    FrameGraphBuilder& builder,
	    const SceneRenderTargets& sceneTargets,
	    const LightingRenderTargets& lighting,
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
