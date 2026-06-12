#include "../PCH.h"
#include "Passes/DirectLightingPass.h"

#include "Core/Public/Diagnostics/Trace.h"
#include "Core/Public/Math/MathUtils.h"
#include "Frame/RenderViewData.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "Diagnostics/PassExecutionDiagnostics.h"
#include "Passes/PassUtilities.h"
#include "Passes/ShaderPass.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "RayTracing/RayTracedShadowPassData.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "Renderer/Public/FrameGraph/FrameGraphAccelerationStructureHandle.h"

#include <cassert>

DirectLightingPass::DirectLightingPass(const ComputePassPipelineRuntime& runtime) noexcept : m_runtime(runtime) {}

const DirectLightingPass::ParameterMetadata& DirectLightingPass::GetParameterMetadata() noexcept
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

ShaderPackageDefinition DirectLightingPass::DescribeShaderPackage() noexcept
{
	return ShaderPackageDefinition{
	    .PackageId = PassName,
	    .BindingLayoutId = PassName,
	    .ExpectedStages = ShaderStageMask::Compute,
	    .RequiredFeatures = CookedShaderPackageFeatureFlags::UsesInlineRayQuery | CookedShaderPackageFeatureFlags::UsesAccelerationStructure};
}

void DirectLightingPass::DeclareResources(
    FrameGraphBuilder& builder,
    const LightingRenderTargets& lighting,
    const GBufferRenderTargets& gbuffer,
    FrameGraphAccelerationStructureHandle sceneTlas,
    ParameterInstance& parameters)
{
	parameters->DirectDiffuse = builder.CreateUAV(lighting.DirectDiffuse);
	parameters->DirectSpecular = builder.CreateUAV(lighting.DirectSpecular);
	parameters->DirectSubsurface = builder.CreateUAV(lighting.DirectSubsurface);
	parameters->GBufferBaseColor = builder.CreateSRV(gbuffer.BaseColor);
	parameters->GBufferNormal = builder.CreateSRV(gbuffer.Normal);
	parameters->GBufferMaterial = builder.CreateSRV(gbuffer.Material);
	parameters->GBufferSubsurface = builder.CreateSRV(gbuffer.Subsurface);
	parameters->GBufferDeviceZ = builder.CreateSRV(gbuffer.DeviceZ);
	parameters->SceneTlas = builder.Read(sceneTlas);
}

void DirectLightingPass::SetParameters(
    ParameterInstance& parameters,
    const RenderViewData& viewData,
    const PassRuntimeServices& passRuntimeServices,
    bool hasSceneTlas) const
{
	parameters->PerFrame = passRuntimeServices.HardwareInterface.GetPerFrameConstantData();
	parameters->PerView = viewData.perViewData;
	parameters->RayTracedShadows = RayTracedShadowPassData::Build(passRuntimeServices.RayTracing, hasSceneTlas);
	const bool valid = parameters.Sync();
	assert(valid);
}

void DirectLightingPass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
	SPARKLE_GPU_PASS_SCOPE(context.Diagnostics, "Renderer.DirectLighting.Execute");

	SetParameters(parameters, context.Frame.mainView, context.RuntimeServices, context.Frame.rayTracingScene.HasTraceableInstances());
	const ComputeDispatchDesc dispatch{
	    MathUtils::DivideRoundUp(static_cast<std::uint32_t>(context.Frame.mainView.viewport.Width), ThreadGroupSizeX),
	    MathUtils::DivideRoundUp(static_cast<std::uint32_t>(context.Frame.mainView.viewport.Height), ThreadGroupSizeY),
	    1};
	const bool dispatched = PassUtilities::DispatchComputePassWithRuntime<DirectLightingPass>(
	    context.Resources,
	    context.Commands,
	    context.RuntimeServices.HardwareInterface,
	    m_runtime,
	    parameters,
	    dispatch,
	    PassName);
	assert(dispatched);
}
