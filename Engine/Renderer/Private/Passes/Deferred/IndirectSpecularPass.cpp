#include "../../PCH.h"
#include "Passes/Deferred/IndirectSpecularPass.h"

#include "Core/Public/Diagnostics/Trace.h"
#include "Core/Public/Math/MathUtils.h"
#include "Diagnostics/PassExecutionDiagnostics.h"
#include "Frame/Core/FrameContext.h"
#include "Frame/Core/RenderViewData.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "Pipeline/PassBindingOverrides.h"
#include "Passes/Core/PassUtilities.h"
#include "Passes/Bindings/MaterialTextureTablePassBinding.h"
#include "Passes/Bindings/RayTracingHitDataPassBinding.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "Passes/Core/ShaderPass.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "RayTracing/Effects/IndirectSpecular/IndirectSpecularPassData.h"
#include "RayTracing/Scene/RenderRayTracingPassServices.h"
#include "RayTracing/Effects/IndirectSpecular/IndirectSpecularRuntimeDiagnostics.h"
#include "RayTracing/Effects/IndirectSpecular/IndirectSpecularSettings.h"
#include "RayTracing/Scene/RayTracingSceneTlasShaderAccessMode.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"
#include "RHI/Public/Samplers/RhiSamplerDesc.h"

#include <cassert>

namespace
{
	constexpr const char* DispatchTimingLabel = "Indirect Specular Ray Query";

	IndirectSpecularSettings ResolveSettings(const PassRuntimeServices& services) noexcept
	{
		const RenderRayTracingPassServices* rayTracingServices = services.RayTracing;
		if (rayTracingServices != nullptr && rayTracingServices->IndirectSpecularSettings != nullptr)
		{
			return *rayTracingServices->IndirectSpecularSettings;
		}

		return BuildIndirectSpecularSettingsFromCVars();
	}

	void PublishStatus(
	    IndirectSpecularStatusReason status,
	    const IndirectSpecularSettings& settings,
	    bool hitDataAvailable,
	    std::uint32_t hitInstanceCount,
	    std::uint32_t hitMaterialCount) noexcept
	{
		IndirectSpecularRuntimeDiagnostics::Publish(
		    IndirectSpecularRuntimeDiagnosticsSnapshot{
		        .Status = status,
		        .Enabled = settings.Enabled,
		        .SampleMode = settings.SampleMode,
		        .DebugMode = settings.DebugMode,
		        .MaxDistance = settings.MaxDistance,
		        .HitDataAvailable = hitDataAvailable,
		        .HitInstanceCount = hitInstanceCount,
		        .HitMaterialCount = hitMaterialCount});
	}

	void LogStatusChange(const IndirectSpecularRuntimeDiagnosticsSnapshot& snapshot) noexcept
	{
		static IndirectSpecularStatusReason s_lastStatus = IndirectSpecularStatusReason::NotEvaluated;
		static bool s_logged = false;
		if (s_logged && s_lastStatus == snapshot.Status)
		{
			return;
		}

		s_logged = true;
		s_lastStatus = snapshot.Status;
		const std::shared_ptr<spdlog::logger> logger = Logging::GetOrCreateLogger("Renderer.IndirectSpecular");
		SPDLOG_LOGGER_INFO(
	    logger,
	    "Indirect specular status: reason={} enabled={} sampleMode={} debugMode={} maxDistance={} hitData={} "
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
	    IndirectSpecularStatusReason status,
	    const IndirectSpecularSettings& settings,
	    bool hitDataAvailable,
	    std::uint32_t hitInstanceCount,
	    std::uint32_t hitMaterialCount) noexcept
	{
		PublishStatus(status, settings, hitDataAvailable, hitInstanceCount, hitMaterialCount);
		LogStatusChange(IndirectSpecularRuntimeDiagnostics::Capture());
	}
}

IndirectSpecularPass::IndirectSpecularPass(const ComputePassPipelineRuntime& runtime) noexcept : m_runtime(runtime) {}

const IndirectSpecularPass::ParameterMetadata& IndirectSpecularPass::GetParameterMetadata() noexcept
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

const RenderPassDefinition& IndirectSpecularPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition{
	    .PassName = PassName,
	    .PackageDeclarationName = "IndirectSpecularShaderPackage",
	    .ShaderPackage =
	        ShaderPackageDefinition{
	            .PackageId = RendererShaderPackages::IndirectSpecular.data(),
	            .BindingLayoutId = RendererShaderPackages::IndirectSpecular.data(),
	            .ExpectedStages = ShaderStageMask::Compute,
	            .RequiredFeatures =
	                CookedShaderPackageFeatureFlags::UsesInlineRayQuery |
	                CookedShaderPackageFeatureFlags::UsesAccelerationStructure |
	                CookedShaderPackageFeatureFlags::UsesDescriptorIndexing},
	    .PipelineKind = RenderPassDefinitionPipelineKind::Compute,
	    .BindingLayoutDebugName = L"IndirectSpecular_BindingLayout",
	    .PipelineStateDebugName = L"IndirectSpecular_PipelineState"};
	return definition;
}

void IndirectSpecularPass::DeclareResources(
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

void IndirectSpecularPass::SetParameters(
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

void IndirectSpecularPass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
	const IndirectSpecularSettings settings = ResolveSettings(context.RuntimeServices);
	const bool hitDataAvailable = RayTracingHitDataPassBinding::IsAvailable(context.Frame);
	const std::uint32_t hitInstanceCount = context.Frame.rayTracingHitData.GetInstanceCount();
	const std::uint32_t hitMaterialCount = context.Frame.rayTracingHitData.GetMaterialCount();
	if (!settings.Enabled)
	{
		PublishAndLogStatus(IndirectSpecularStatusReason::Disabled, settings, hitDataAvailable, hitInstanceCount, hitMaterialCount);
		return;
	}

	if (!context.Frame.rayTracingScene.HasBoundTlas())
	{
		PublishAndLogStatus(IndirectSpecularStatusReason::MissingTlas, settings, hitDataAvailable, hitInstanceCount, hitMaterialCount);
		return;
	}
	if (context.Frame.rayTracingScene.TlasShaderAccessMode != RayTracingSceneTlasShaderAccessMode::Descriptor)
	{
		PublishAndLogStatus(IndirectSpecularStatusReason::Unsupported, settings, hitDataAvailable, hitInstanceCount, hitMaterialCount);
		return;
	}

	const bool materialTextureTableAvailable = MaterialTextureTablePassBinding::Bind(parameters, context.Frame);
	if (!materialTextureTableAvailable)
	{
		PublishAndLogStatus(IndirectSpecularStatusReason::Unsupported, settings, hitDataAvailable, hitInstanceCount, hitMaterialCount);
		return;
	}

	SetParameters(parameters, context.Frame.mainView, context.RuntimeServices);
	parameters->IndirectSpecularConstants = IndirectSpecularPassData::Build(
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
	    hitDataAvailable ? IndirectSpecularStatusReason::Running : IndirectSpecularStatusReason::MissingHitData,
	    settings,
	    hitDataAvailable,
	    hitInstanceCount,
	    hitMaterialCount);
	const bool dispatched = [&]() noexcept
	{
		SPARKLE_GPU_SCOPE(context.Diagnostics, DispatchTimingLabel);
		return PassUtilities::DispatchAvailableComputePassWithRuntime<IndirectSpecularPass>(
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
