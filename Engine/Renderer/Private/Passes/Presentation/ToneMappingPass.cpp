#include "../../PCH.h"
#include "Passes/Presentation/ToneMappingPass.h"

#include "Frame/Core/FrameRenderFormats.h"
#include "Frame/Presentation/ToneMappingSettings.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "Passes/Core/ComputePassUtilities.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

ToneMappingPass::ToneMappingPass(const ComputePassPipelineRuntime& runtime) noexcept : m_runtime(runtime) {}

const ToneMappingPass::ParameterMetadata& ToneMappingPass::GetParameterMetadata() noexcept
{
	return ComputePassUtilities::BuildParameterMetadata<ToneMappingPass>();
}

const RenderPassDefinition& ToneMappingPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassUtilities::BuildDefinition(
	    PassName,
	    RendererShaderPackages::ToneMapping,
	    L"ToneMapping_BindingLayout",
	    L"ToneMapping_PipelineState");
	return definition;
}

void ToneMappingPass::DeclareResources(
    FrameGraphBuilder& builder,
    FrameGraphTextureHandle sceneColor,
    FrameGraphTextureHandle exposure,
    FrameGraphTextureHandle toneMappedColor,
    ParameterInstance& parameters)
{
	parameters->ToneMappedColor = builder.CreateUAV(toneMappedColor);
	parameters->SceneColor = builder.CreateSRV(sceneColor);
	parameters->ExposureTexture = builder.CreateSRV(exposure);
}

void ToneMappingPass::Execute(
    PassExecutionContext& context,
    ParameterInstance& parameters,
    std::uint32_t outputWidth,
    std::uint32_t outputHeight) const
{
	parameters->ToneMappingConstants = BuildToneMappingUniformDataFromCVars();
	ComputePassUtilities::DispatchSized<ToneMappingPass>(context, m_runtime, parameters, outputWidth, outputHeight);
}
