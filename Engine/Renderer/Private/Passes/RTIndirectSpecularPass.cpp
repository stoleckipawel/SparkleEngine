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
#include "Passes/MaterialTextureTablePassBinding.h"
#include "Passes/RayTracingHitDataPassBinding.h"
#include "Passes/RenderPassDefinition.h"
#include "Passes/ShaderPass.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "RayTracing/RTIndirectSpecularPassData.h"
#include "RayTracing/RenderRayTracingPassServices.h"
#include "RayTracing/RTIndirectSpecularRuntimeDiagnostics.h"
#include "RayTracing/RTIndirectSpecularSettings.h"
#include "RayTracing/RayTracingSceneTlasShaderAccessMode.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"
#include "RHI/Public/Samplers/RhiSamplerDesc.h"

#include <cassert>

namespace
{
	constexpr const char* DispatchTimingLabel = "RT Indirect Specular Ray Query";

	RTIndirectSpecularSettings ResolveSettings(const PassRuntimeServices& services) noexcept
	{
		const RenderRayTracingPassServices* rayTracingServices = services.RayTracing;
		if (rayTracingServices != nullptr && rayTracingServices->IndirectSpecularSettings != nullptr)
		{
			return *rayTracingServices->IndirectSpecularSettings;
		}

		return BuildRTIndirectSpecularSettingsFromCVars();
	}

	void PublishStatus(
	    RTIndirectSpecularStatusReason status,
	    const RTIndirectSpecularSettings& settings,
	    bool hitDataAvailable,
	    std::uint32_t hitInstanceCount,
	    std::uint32_t hitMaterialCount) noexcept
	{
		RTIndirectSpecularRuntimeDiagnostics::Publish(
		    RTIndirectSpecularRuntimeDiagnosticsSnapshot{
		        .Status = status,
		        .Enabled = settings.Enabled,
		        .SampleMode = settings.SampleMode,
		        .DebugMode = settings.DebugMode,
		        .MaxDistance = settings.MaxDistance,
		        .HitDataAvailable = hitDataAvailable,
		        .HitInstanceCount = hitInstanceCount,
		        .HitMaterialCount = hitMaterialCount});
	}

	void LogStatusChange(const RTIndirectSpecularRuntimeDiagnosticsSnapshot& snapshot) noexcept
	{
		static RTIndirectSpecularStatusReason s_lastStatus = RTIndirectSpecularStatusReason::NotEvaluated;
		static bool s_logged = false;
		if (s_logged && s_lastStatus == snapshot.Status)
		{
			return;
		}

		s_logged = true;
		s_lastStatus = snapshot.Status;
		const std::shared_ptr<spdlog::logger> logger = Logging::GetOrCreateLogger("Renderer.RTIndirectSpecular");
		SPDLOG_LOGGER_INFO(
		    logger,
		    "RT indirect specular status: reason={} enabled={} sampleMode={} debugMode={} maxDistance={} hitData={} "
		    "hitInstances={} hitMaterials={}",
		    snapshot.StatusReason,
		    snapshot.Enabled ? "true" : "false",
		    static_cast<std::uint32_t>(snapshot.SampleMode),
		    static_cast<std::uint32_t>(snapshot.DebugMode),
		    snapshot.MaxDistance,
		    snapshot.HitDataAvailable ? "true" : "false",
		    snapshot.HitInstanceCount,
		    snapshot.HitMaterialCount);
	}

	void PublishAndLogStatus(
	    RTIndirectSpecularStatusReason status,
	    const RTIndirectSpecularSettings& settings,
	    bool hitDataAvailable,
	    std::uint32_t hitInstanceCount,
	    std::uint32_t hitMaterialCount) noexcept
	{
		PublishStatus(status, settings, hitDataAvailable, hitInstanceCount, hitMaterialCount);
		LogStatusChange(RTIndirectSpecularRuntimeDiagnostics::Capture());
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
	                CookedShaderPackageFeatureFlags::UsesInlineRayQuery |
	                CookedShaderPackageFeatureFlags::UsesAccelerationStructure |
	                CookedShaderPackageFeatureFlags::UsesDescriptorIndexing},
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
	RayTracingHitDataPassBinding::DeclareResources(builder, parameters);
}

void RTIndirectSpecularPass::SetParameters(
    ParameterInstance& parameters,
    const RenderViewData& viewData,
    const PassRuntimeServices& passRuntimeServices) const
{
	parameters->PerFrame = passRuntimeServices.HardwareInterface.GetUploadService().GetPerFrameConstantData();
	parameters->PerView = viewData.perViewData;
	parameters->MaterialTextureSampler =
	    RhiSamplerDesc{
	        .MinMagFilter = RhiSamplerMinMagFilter::Linear,
	        .MipFilter = RhiSamplerMipFilter::Linear,
	        .Address = MakeRhiSamplerAddressModes(RhiSamplerAddressMode::Wrap),
	        .MaxAnisotropy = RhiSamplerAnisotropy::X1};
}

void RTIndirectSpecularPass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
	SPARKLE_GPU_PASS_SCOPE(context.Diagnostics, "Renderer.RTIndirectSpecular.Execute");

	const RTIndirectSpecularSettings settings = ResolveSettings(context.RuntimeServices);
	const bool hitDataAvailable = RayTracingHitDataPassBinding::IsAvailable(context.Frame);
	const std::uint32_t hitInstanceCount = context.Frame.rayTracingHitData.GetInstanceCount();
	const std::uint32_t hitMaterialCount = context.Frame.rayTracingHitData.GetMaterialCount();
	if (!settings.Enabled)
	{
		PublishAndLogStatus(RTIndirectSpecularStatusReason::Disabled, settings, hitDataAvailable, hitInstanceCount, hitMaterialCount);
		return;
	}

	if (!context.Frame.rayTracingScene.HasBoundTlas())
	{
		PublishAndLogStatus(RTIndirectSpecularStatusReason::MissingTlas, settings, hitDataAvailable, hitInstanceCount, hitMaterialCount);
		return;
	}
	if (context.Frame.rayTracingScene.TlasShaderAccessMode != RayTracingSceneTlasShaderAccessMode::Descriptor)
	{
		PublishAndLogStatus(RTIndirectSpecularStatusReason::Unsupported, settings, hitDataAvailable, hitInstanceCount, hitMaterialCount);
		return;
	}

	const bool materialTextureTableAvailable = MaterialTextureTablePassBinding::Bind(parameters, context.Frame);
	if (!materialTextureTableAvailable)
	{
		PublishAndLogStatus(RTIndirectSpecularStatusReason::Unsupported, settings, hitDataAvailable, hitInstanceCount, hitMaterialCount);
		return;
	}

	SetParameters(parameters, context.Frame.mainView, context.RuntimeServices);
	parameters->RTIndirectSpecular = RTIndirectSpecularPassData::Build(
	    settings,
	    hitDataAvailable,
	    hitInstanceCount,
	    hitMaterialCount,
	    materialTextureTableAvailable,
	    context.Frame.sceneData.materialTextureTableDescriptorCount,
	    MaterialTextureTableFixedCapacity);
	const bool valid = parameters.Sync();
	assert(valid);

	PassBindingOverrides overrides;
	if (hitDataAvailable)
	{
		RayTracingHitDataPassBinding::Bind(overrides, context.Frame);
	}

	const ComputeDispatchDesc dispatch{
	    MathUtils::DivideRoundUp(static_cast<std::uint32_t>(context.Frame.mainView.viewport.Width), ThreadGroupSizeX),
	    MathUtils::DivideRoundUp(static_cast<std::uint32_t>(context.Frame.mainView.viewport.Height), ThreadGroupSizeY),
	    1};
	PublishAndLogStatus(
	    hitDataAvailable ? RTIndirectSpecularStatusReason::Running : RTIndirectSpecularStatusReason::MissingHitData,
	    settings,
	    hitDataAvailable,
	    hitInstanceCount,
	    hitMaterialCount);
	const bool dispatched = [&]() noexcept
	{
		auto rayQueryScope = context.Diagnostics.BeginGpuEvent(DispatchTimingLabel);
		auto rayQueryTimer = context.Diagnostics.BeginTimer(DispatchTimingLabel);
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
