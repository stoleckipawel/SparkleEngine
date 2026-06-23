#include "../../PCH.h"
#include "Passes/Deferred/IndirectLightingPass.h"

#include "Core/Public/Diagnostics/Trace.h"
#include "Core/Public/Math/MathUtils.h"
#include "Diagnostics/PassExecutionDiagnostics.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "Passes/Core/PassUtilities.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "Passes/Core/ShaderPass.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

#include <cassert>

IndirectLightingPass::IndirectLightingPass(const ComputePassPipelineRuntime& runtime) noexcept : m_runtime(runtime) {}

const IndirectLightingPass::ParameterMetadata& IndirectLightingPass::GetParameterMetadata() noexcept
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

const RenderPassDefinition& IndirectLightingPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition{
	    .PassName = PassName,
	    .PackageDeclarationName = "IndirectLightingShaderPackage",
	    .ShaderPackage = ShaderPackageDefinition{
	        .PackageId = RendererShaderPackages::IndirectLighting.data(),
	        .BindingLayoutId = RendererShaderPackages::IndirectLighting.data(),
	        .ExpectedStages = ShaderStageMask::Compute},
	    .PipelineKind = RenderPassDefinitionPipelineKind::Compute,
	    .BindingLayoutDebugName = L"IndirectLighting_BindingLayout",
	    .PipelineStateDebugName = L"IndirectLighting_PipelineState"};
	return definition;
}

void IndirectLightingPass::DeclareResources(
    FrameGraphBuilder& builder,
    const LightingRenderTargets& lighting,
    const GBufferRenderTargets& gbuffer,
    ParameterInstance& parameters)
{
	parameters->IndirectDiffuse = builder.CreateUAV(lighting.IndirectDiffuse);
	parameters->GBufferNormal = builder.CreateSRV(gbuffer.Normal);
	parameters->GBufferDeviceZ = builder.CreateSRV(gbuffer.DeviceZ);
	parameters->SamplerLinearNoMipClamp = RhiSamplerDesc{
	    .MinMagFilter = RhiSamplerMinMagFilter::Linear,
	    .MipFilter = RhiSamplerMipFilter::Point,
	    .Address = MakeRhiSamplerAddressModes(RhiSamplerAddressMode::Clamp)};
}

void IndirectLightingPass::SetParameters(ParameterInstance& parameters, const PassRuntimeServices& passRuntimeServices) const
{
	parameters->SamplerLinearNoMipClamp = RhiSamplerDesc{
	    .MinMagFilter = RhiSamplerMinMagFilter::Linear,
	    .MipFilter = RhiSamplerMipFilter::Point,
	    .Address = MakeRhiSamplerAddressModes(RhiSamplerAddressMode::Clamp)};
	parameters->SkyTexture = m_environmentMapBinding.GetTextureBinding(passRuntimeServices);
	const bool valid = parameters.Sync();
	assert(valid);
}

void IndirectLightingPass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
	SetParameters(parameters, context.RuntimeServices);
	const ComputeDispatchDesc dispatch{
	    MathUtils::DivideRoundUp(static_cast<std::uint32_t>(context.Frame.mainView.viewport.Width), ThreadGroupSizeX),
	    MathUtils::DivideRoundUp(static_cast<std::uint32_t>(context.Frame.mainView.viewport.Height), ThreadGroupSizeY),
	    1};
	const bool dispatched = PassUtilities::DispatchComputePassWithRuntime<IndirectLightingPass>(
	    context.Resources,
	    context.Commands,
	    context.RuntimeServices.HardwareInterface,
	    m_runtime,
	    parameters.GetPassParameterSet(),
	    dispatch,
	    PassName);
	assert(dispatched);
}
