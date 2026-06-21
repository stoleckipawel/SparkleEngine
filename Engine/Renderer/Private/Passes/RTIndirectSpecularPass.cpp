#include "../PCH.h"
#include "Passes/RTIndirectSpecularPass.h"

#include "Core/Public/Diagnostics/Trace.h"
#include "Core/Public/Math/MathUtils.h"
#include "Diagnostics/PassExecutionDiagnostics.h"
#include "Frame/RenderViewData.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "Passes/PassUtilities.h"
#include "Passes/RenderPassDefinition.h"
#include "Passes/ShaderPass.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "RayTracing/RTIndirectSpecularPassData.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

#include <cassert>

RTIndirectSpecularPass::RTIndirectSpecularPass(const ComputePassPipelineRuntime& runtime) noexcept : m_runtime(runtime) {}

const RTIndirectSpecularPass::ParameterMetadata& RTIndirectSpecularPass::GetParameterMetadata() noexcept
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

const RenderPassDefinition& RTIndirectSpecularPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition{
	    .PassName = PassName,
	    .PackageDeclarationName = "RTIndirectSpecularShaderPackage",
	    .ShaderPackage =
	        ShaderPackageDefinition{
	            .PackageId = RendererShaderPackages::RTIndirectSpecular.data(),
	            .BindingLayoutId = RendererShaderPackages::RTIndirectSpecular.data(),
	            .ExpectedStages = ShaderStageMask::Compute,
	            .RequiredFeatures =
	                CookedShaderPackageFeatureFlags::UsesInlineRayQuery | CookedShaderPackageFeatureFlags::UsesAccelerationStructure},
	    .PipelineKind = RenderPassDefinitionPipelineKind::Compute,
	    .BindingLayoutDebugName = L"RTIndirectSpecular_BindingLayout",
	    .PipelineStateDebugName = L"RTIndirectSpecular_PipelineState"};
	return definition;
}

void RTIndirectSpecularPass::DeclareResources(
    FrameGraphBuilder& builder,
    const LightingRenderTargets& lighting,
    const GBufferRenderTargets& gbuffer,
    FrameGraphAccelerationStructureHandle sceneTlas,
    ParameterInstance& parameters)
{
	parameters->IndirectSpecular = builder.CreateUAV(lighting.IndirectSpecular);
	parameters->SceneTlas = builder.Read(sceneTlas);
	parameters->GBufferBaseColor = builder.CreateSRV(gbuffer.BaseColor);
	parameters->GBufferNormal = builder.CreateSRV(gbuffer.Normal);
	parameters->GBufferMaterial = builder.CreateSRV(gbuffer.Material);
	parameters->GBufferDeviceZ = builder.CreateSRV(gbuffer.DeviceZ);
}

void RTIndirectSpecularPass::SetParameters(
    ParameterInstance& parameters,
    const RenderViewData& viewData,
    const PassRuntimeServices& passRuntimeServices) const
{
	parameters->PerFrame = passRuntimeServices.HardwareInterface.GetUploadService().GetPerFrameConstantData();
	parameters->PerView = viewData.perViewData;
	parameters->RTIndirectSpecular = RTIndirectSpecularPassData::Build();
	const bool valid = parameters.Sync();
	assert(valid);
}

void RTIndirectSpecularPass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
	SPARKLE_GPU_PASS_SCOPE(context.Diagnostics, "Renderer.RTIndirectSpecular.Execute");

	if (!context.Frame.rayTracingScene.HasBoundTlas())
	{
		return;
	}

	SetParameters(parameters, context.Frame.mainView, context.RuntimeServices);
	const ComputeDispatchDesc dispatch{
	    MathUtils::DivideRoundUp(static_cast<std::uint32_t>(context.Frame.mainView.viewport.Width), ThreadGroupSizeX),
	    MathUtils::DivideRoundUp(static_cast<std::uint32_t>(context.Frame.mainView.viewport.Height), ThreadGroupSizeY),
	    1};
	const bool dispatched = [&]() noexcept
	{
		auto rayQueryScope = context.Diagnostics.BeginGpuEvent("RT Indirect Specular Mirror Ray Query");
		auto rayQueryTimer = context.Diagnostics.BeginTimer("RT Indirect Specular Mirror Ray Query");
		return PassUtilities::DispatchComputePassWithRuntime<RTIndirectSpecularPass>(
		    context.Resources,
		    context.Commands,
		    context.RuntimeServices.HardwareInterface,
		    m_runtime,
		    parameters,
		    dispatch,
		    PassName);
	}();
	assert(dispatched);
}
