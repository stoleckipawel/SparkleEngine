#include "../PCH.h"
#include "Passes/ComputeClearPass.h"

#include "Commands/RenderCommandContext.h"
#include "Diagnostics/PassExecutionDiagnostics.h"
#include "Core/Public/Diagnostics/Trace.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "Core/Public/Math/MathUtils.h"
#include "Passes/PassUtilities.h"
#include "Passes/RenderPassDefinition.h"
#include "Passes/ShaderPass.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "RHI/Public/ShaderParameters/PassParameterLayout.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

#include <cassert>

ComputeClearPass::ComputeClearPass(const ComputePassPipelineRuntime& runtime) noexcept : m_runtime(runtime) {}

const ComputeClearPass::ParameterMetadata& ComputeClearPass::GetParameterMetadata() noexcept
{
	static const ParameterMetadata metadata = []
	{
		const ParameterMetadata localMetadata = ShaderParameterStructBuilder<Parameters>::BuildMetadata(PassName);
		const bool valid = ValidateShaderPassLayout(localMetadata.GetLayout(), ShaderPassKind::Compute, PassName);
		assert(valid);
		return localMetadata;
	}();
	return metadata;
}

const PassParameterLayout& ComputeClearPass::GetParameterLayout() noexcept
{
	return GetParameterMetadata().GetLayout();
}

const RenderPassDefinition& ComputeClearPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition{
	    .PassName = PassName,
	    .PackageDeclarationName = "ComputeClearShaderPackage",
	    .ShaderPackage = ShaderPackageDefinition{
	        .PackageId = RendererShaderPackages::ComputeClear.data(),
	        .BindingLayoutId = RendererShaderPackages::ComputeClear.data(),
	        .ExpectedStages = ShaderStageMask::Compute},
	    .PipelineKind = RenderPassDefinitionPipelineKind::Compute,
	    .BindingLayoutDebugName = L"ComputeClear_BindingLayout",
	    .PipelineStateDebugName = L"ComputeClear_PipelineState"};
	return definition;
}

void ComputeClearPass::DeclareResources(FrameGraphBuilder& builder, FrameGraphTextureHandle outputTexture, ParameterInstance& parameters)
{
	parameters->Output = builder.CreateUAV(outputTexture);
}

void ComputeClearPass::Execute(
	PassExecutionContext& context,
	const ComputeClearPass::ParameterInstance& parameters,
	std::uint32_t width,
	std::uint32_t height) const noexcept
{
	SPARKLE_GPU_PASS_SCOPE(context.Diagnostics, "Renderer.ComputeClear.Execute");

	const ComputeDispatchDesc dispatch{
	    MathUtils::DivideRoundUp(width, ThreadGroupSizeX),
	    MathUtils::DivideRoundUp(height, ThreadGroupSizeY),
	    1};
	const bool dispatched = PassUtilities::DispatchComputePassWithRuntime<ComputeClearPass>(
	    context.Resources,
	    context.Commands,
	    context.RuntimeServices.HardwareInterface,
	    m_runtime,
	    parameters,
	    dispatch,
	    PassName);
	assert(dispatched);
}
