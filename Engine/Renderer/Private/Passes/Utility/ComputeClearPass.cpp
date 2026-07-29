#include "../../PCH.h"
#include "Passes/Utility/ComputeClearPass.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "Passes/Core/ComputePassOperations.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "RHI/Public/ShaderParameters/PassParameterLayout.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

ComputeClearPass::ComputeClearPass(const ComputePassPipelineRuntime& runtime) noexcept : m_runtime(runtime) {}

const ComputeClearPass::ParameterMetadata& ComputeClearPass::GetParameterMetadata() noexcept
{
	return ComputePassOperations::BuildParameterMetadata<ComputeClearPass>();
}

const PassParameterLayout& ComputeClearPass::GetParameterLayout() noexcept
{
	return GetParameterMetadata().GetLayout();
}

const RenderPassDefinition& ComputeClearPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassOperations::BuildDefinition(
	    PassName,
	    RendererShaderPackages::ComputeClear,
	    L"ComputeClear_BindingLayout",
	    L"ComputeClear_Pipeline");
	return definition;
}

void ComputeClearPass::Execute(
    PassExecutionContext& context,
    const ComputeClearPass::ParameterInstance& parameters,
    std::uint32_t width,
    std::uint32_t height) const noexcept
{
	ComputePassOperations::DispatchSized<ComputeClearPass>(context, m_runtime, parameters, width, height);
}

void AddComputeClearPass(
    FrameGraphBuilder& builder,
    std::string_view passName,
    FrameGraphTextureHandle outputTexture,
    RenderViewportExtent outputExtent)
{
	auto& parameters = builder.AllocParameters<ComputeClearPass::Parameters>();
	parameters->Output = builder.CreateUAV(outputTexture);
	builder.Dispatch<ComputeClearPass>(
	    passName,
	    parameters,
	    outputExtent.Width,
	    outputExtent.Height);
}
