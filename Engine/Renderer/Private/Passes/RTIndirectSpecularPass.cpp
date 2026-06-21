#include "../PCH.h"
#include "Passes/RTIndirectSpecularPass.h"

#include "Core/Public/Diagnostics/Trace.h"
#include "Core/Public/Math/MathUtils.h"
#include "Diagnostics/PassExecutionDiagnostics.h"
#include "Frame/FrameContext.h"
#include "Frame/RenderViewData.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "Pipeline/PassBindingOverrides.h"
#include "Passes/PassUtilities.h"
#include "Passes/RenderPassDefinition.h"
#include "Passes/ShaderPass.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "RayTracing/RTIndirectSpecularPassData.h"
#include "Renderer/Public/FrameGraph/FrameGraphBufferDesc.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"

#include <cassert>

namespace
{
	template <typename TData>
	FrameGraphBufferHandle CreatePlaceholderBuffer(FrameGraphBuilder& builder, const char* name)
	{
		return builder.CreateBuffer(FrameGraphBufferDesc::Create(name, sizeof(TData), static_cast<std::uint32_t>(sizeof(TData))));
	}
}

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
	parameters->RTIndirectSpecularHitVertices =
	    builder.CreateSRV<RTIndirectSpecularHitVertex>(CreatePlaceholderBuffer<RTIndirectSpecularHitVertex>(builder, "RTIndirectSpecularHitVerticesPlaceholder"));
	parameters->RTIndirectSpecularHitIndices =
	    builder.CreateSRV<std::uint32_t>(CreatePlaceholderBuffer<std::uint32_t>(builder, "RTIndirectSpecularHitIndicesPlaceholder"));
	parameters->RTIndirectSpecularHitInstances =
	    builder.CreateSRV<RTIndirectSpecularHitInstance>(CreatePlaceholderBuffer<RTIndirectSpecularHitInstance>(builder, "RTIndirectSpecularHitInstancesPlaceholder"));
	parameters->RTIndirectSpecularHitMaterials =
	    builder.CreateSRV<RTIndirectSpecularHitMaterial>(CreatePlaceholderBuffer<RTIndirectSpecularHitMaterial>(builder, "RTIndirectSpecularHitMaterialsPlaceholder"));
	parameters->MeshInstances =
	    builder.CreateSRV<MeshInstanceData>(CreatePlaceholderBuffer<MeshInstanceData>(builder, "RTIndirectSpecularMeshInstancesPlaceholder"));
}

void RTIndirectSpecularPass::SetParameters(
    ParameterInstance& parameters,
    const RenderViewData& viewData,
    const PassRuntimeServices& passRuntimeServices) const
{
	parameters->PerFrame = passRuntimeServices.HardwareInterface.GetUploadService().GetPerFrameConstantData();
	parameters->PerView = viewData.perViewData;
}

void RTIndirectSpecularPass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
	SPARKLE_GPU_PASS_SCOPE(context.Diagnostics, "Renderer.RTIndirectSpecular.Execute");

	if (!context.Frame.rayTracingScene.HasBoundTlas())
	{
		return;
	}

	SetParameters(parameters, context.Frame.mainView, context.RuntimeServices);
	parameters->RTIndirectSpecular = RTIndirectSpecularPassData::Build(
	    context.Frame.rtIndirectSpecularHitData.IsValid() && context.Frame.meshInstances.IsValid(),
	    context.Frame.rtIndirectSpecularHitData.GetInstanceCount(),
	    context.Frame.rtIndirectSpecularHitData.GetMaterialCount());
	const bool valid = parameters.Sync();
	assert(valid);

	PassBindingOverrides overrides;
	if (context.Frame.rtIndirectSpecularHitData.IsValid() && context.Frame.meshInstances.IsValid())
	{
		overrides.SetDescriptorTable("RTIndirectSpecularHitVertices", context.Frame.rtIndirectSpecularHitData.GetVertexShaderResourceView());
		overrides.SetDescriptorTable("RTIndirectSpecularHitIndices", context.Frame.rtIndirectSpecularHitData.GetIndexShaderResourceView());
		overrides.SetDescriptorTable("RTIndirectSpecularHitInstances", context.Frame.rtIndirectSpecularHitData.GetInstanceShaderResourceView());
		overrides.SetDescriptorTable("RTIndirectSpecularHitMaterials", context.Frame.rtIndirectSpecularHitData.GetMaterialShaderResourceView());
		overrides.SetDescriptorTable("MeshInstances", context.Frame.meshInstances.GetShaderResourceView());
	}

	const ComputeDispatchDesc dispatch{
	    MathUtils::DivideRoundUp(static_cast<std::uint32_t>(context.Frame.mainView.viewport.Width), ThreadGroupSizeX),
	    MathUtils::DivideRoundUp(static_cast<std::uint32_t>(context.Frame.mainView.viewport.Height), ThreadGroupSizeY),
	    1};
	const bool dispatched = [&]() noexcept
	{
		auto rayQueryScope = context.Diagnostics.BeginGpuEvent("RT Indirect Specular Mirror Ray Query");
		auto rayQueryTimer = context.Diagnostics.BeginTimer("RT Indirect Specular Mirror Ray Query");
		return PassUtilities::DispatchAvailableComputePassWithRuntime<RTIndirectSpecularPass>(
		    context.Resources,
		    context.Commands,
		    context.RuntimeServices.HardwareInterface,
		    m_runtime,
		    parameters.GetPassParameterSet(),
		    dispatch,
		    &overrides,
		    PassName);
	}();
	assert(dispatched);
}
