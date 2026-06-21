#include "PCH.h"
#include "Diagnostics/RendererDiagnosticsCollector.h"

#include "Host/RendererSystemRoot.h"
#include "Pipeline/PipelineStateManager.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "Upscaling/UpscalerProvider.h"
#include "Upscaling/UpscalerSubsystem.h"

#include <numeric>

namespace
{
	RendererMemoryPressureLevel ClassifyUploadPressure(float usageRatio) noexcept
	{
		const RendererMemoryPressureThresholds thresholds;
		if (usageRatio >= thresholds.CriticalRatio)
		{
			return RendererMemoryPressureLevel::Critical;
		}
		if (usageRatio >= thresholds.PressureRatio)
		{
			return RendererMemoryPressureLevel::Pressure;
		}
		if (usageRatio >= thresholds.WatchRatio)
		{
			return RendererMemoryPressureLevel::Watch;
		}
		return RendererMemoryPressureLevel::Normal;
	}

	RendererUploadPressureDiagnosticsSnapshot BuildUploadPressureSnapshot(const RendererMemoryDiagnosticsSnapshot& memory)
	{
		if (!memory.Available)
		{
			return RendererUploadPressureDiagnosticsSnapshot{
			    .Status = ERendererDiagnosticStatus::Unavailable,
			    .Reason = "RHI memory diagnostics are not available."};
		}

		for (const RhiMemoryCategoryStats& category : memory.MemoryUsage.CategoryStats)
		{
			if (category.Category != RhiMemoryCategory::Upload)
			{
				continue;
			}

			const float budgetUsageRatio = category.BudgetBytes != 0u
			                               ? static_cast<float>(category.UsedBytes) / static_cast<float>(category.BudgetBytes)
			                               : 0.0f;
			return RendererUploadPressureDiagnosticsSnapshot{
			    .Status = ERendererDiagnosticStatus::Available,
			    .UsedBytes = category.UsedBytes,
			    .BudgetBytes = category.BudgetBytes,
			    .BudgetUsageRatio = budgetUsageRatio,
			    .Pressure = ClassifyUploadPressure(budgetUsageRatio),
			    .Reason = category.BudgetBytes != 0u ? "" : "Upload memory is tracked without a backend budget."};
		}

		return RendererUploadPressureDiagnosticsSnapshot{
		    .Status = ERendererDiagnosticStatus::Unavailable,
		    .Reason = "No upload memory category was reported by the RHI allocator."};
	}

	std::uint64_t CountAllocatedDescriptors(const RhiDescriptorUsageSnapshot& descriptors) noexcept
	{
		return std::accumulate(
		    descriptors.Allocators.begin(),
		    descriptors.Allocators.end(),
		    std::uint64_t{0},
		    [](std::uint64_t total, const RhiDescriptorAllocatorUsage& allocator)
		    {
			    return total + allocator.Allocated;
		    });
	}

	RendererProviderDiagnosticsSnapshot BuildProviderSnapshot(const UpscalerSubsystem* upscalerSubsystem)
	{
		if (upscalerSubsystem == nullptr)
		{
			return RendererProviderDiagnosticsSnapshot{};
		}

		const UpscalerProviderCapabilities& provider = upscalerSubsystem->GetDiagnostics();
		return RendererProviderDiagnosticsSnapshot{
		    .Status = ERendererDiagnosticStatus::Available,
		    .RequestedProvider = UpscalerProviderKindToString(upscalerSubsystem->GetRequestedProviderKind()),
		    .ActiveProvider = provider.ProviderName.empty() ? std::string(upscalerSubsystem->GetActiveProvider().GetName()) :
		                                                      provider.ProviderName,
		    .Category = RendererProviderCategoryToString(provider.Category),
		    .CapabilityState = RendererProviderCapabilityStateToString(provider.CapabilityState),
		    .FailureDomain = UpscalerProviderFailureDomainToString(provider.FailureDomain),
		    .CanEvaluate = provider.CanEvaluate,
		    .UsesExternalSdk = provider.UsesExternalSdk,
		    .RuntimeVersion = provider.ExternalRuntimeVersion,
		    .RuntimeState = provider.RuntimeState,
		    .ResourceContract = provider.ResourceContractSummary,
		    .Reason = provider.Reason};
	}
}

RendererDiagnosticsSnapshot RendererDiagnosticsCollector::Capture(const RendererSystemRoot& systems)
{
	RendererDiagnosticsSnapshot snapshot;
	if (!systems.HasBackend())
	{
		snapshot.Backend.Status = ERendererDiagnosticStatus::Unavailable;
		snapshot.Backend.BackendName = "Unavailable";
		return snapshot;
	}

	const RenderHardwareInterface& renderHardware = systems.GetRenderHardwareInterface();
	const RhiCapabilities& capabilities = renderHardware.GetCapabilities();
	const RenderDiagnostics& diagnostics = renderHardware.GetDiagnostics();

	snapshot.FrameIndex = renderHardware.GetCurrentFrameIndex();
	snapshot.Backend = RendererBackendDiagnosticsSnapshot{
	    .Status = ERendererDiagnosticStatus::Available,
	    .BackendApi = capabilities.BackendApi,
	    .BackendName = RendererBackendApiToString(capabilities.BackendApi),
	    .Version = capabilities.BackendVersion,
	    .VersionText = FormatRendererBackendVersion(capabilities.BackendVersion),
	    .Adapter = capabilities.ExternalFeatureInterop.Adapter,
	    .DiagnosticsSupport = capabilities.Diagnostics,
	    .MemorySupport = capabilities.MemorySupport,
	    .RuntimeDiagnostics = diagnostics.GetCapabilities()};
	snapshot.Memory = systems.CaptureMemoryDiagnostics();
	snapshot.Descriptors = renderHardware.GetDescriptorService().CaptureDescriptorUsageSnapshot();
	snapshot.UploadPressure = BuildUploadPressureSnapshot(snapshot.Memory);
	snapshot.Pipeline = systems.GetPipelineStateManager().CaptureDiagnosticsSnapshot();
	snapshot.Provider = BuildProviderSnapshot(systems.GetUpscalerSubsystem());

	snapshot.Metrics.push_back(
	    RendererDiagnosticMetric{
	        .Name = "backend.apiVersionOrFeatureLevel",
	        .Origin = ERendererDiagnosticOrigin::RhiBackend,
	        .Status = capabilities.BackendVersion.IsKnown() ? ERendererDiagnosticStatus::Available : ERendererDiagnosticStatus::Unavailable,
	        .Unit = ERendererDiagnosticUnit::Text,
	        .TextValue = snapshot.Backend.VersionText});
			
	snapshot.Metrics.push_back(
	    RendererDiagnosticMetric{
	        .Name = "backend.validationState",
	        .Origin = ERendererDiagnosticOrigin::RhiBackend,
	        .Status = ERendererDiagnosticStatus::Available,
	        .Unit = ERendererDiagnosticUnit::Boolean,
	        .IntegerValue = capabilities.Diagnostics.ValidationEnabled ? 1u : 0u,
	        .TextValue = capabilities.Diagnostics.ValidationEnabled ? "enabled" : "disabled"});

	snapshot.Metrics.push_back(
	    RendererDiagnosticMetric{
	        .Name = "providers.requested",
	        .Origin = ERendererDiagnosticOrigin::RendererProvider,
	        .Status = snapshot.Provider.RequestedProvider.empty() ? ERendererDiagnosticStatus::Unavailable : ERendererDiagnosticStatus::Available,
	        .Unit = ERendererDiagnosticUnit::Text,
	        .TextValue = snapshot.Provider.RequestedProvider});

	snapshot.Metrics.push_back(
	    RendererDiagnosticMetric{
	        .Name = "providers.active",
	        .Origin = ERendererDiagnosticOrigin::RendererProvider,
	        .Status = snapshot.Provider.ActiveProvider.empty() ? ERendererDiagnosticStatus::Unavailable : ERendererDiagnosticStatus::Available,
	        .Unit = ERendererDiagnosticUnit::Text,
	        .TextValue = snapshot.Provider.ActiveProvider});

	snapshot.Metrics.push_back(
	    RendererDiagnosticMetric{
	        .Name = "descriptors.allocated",
	        .Origin = ERendererDiagnosticOrigin::RhiDescriptorService,
	        .Status = snapshot.Descriptors.Allocators.empty() ? ERendererDiagnosticStatus::Unavailable : ERendererDiagnosticStatus::Available,
	        .Unit = ERendererDiagnosticUnit::Count,
	        .IntegerValue = CountAllocatedDescriptors(snapshot.Descriptors)});

	snapshot.Metrics.push_back(
	    RendererDiagnosticMetric{
	        .Name = "pipeline.lazyRuntimeCount",
	        .Origin = ERendererDiagnosticOrigin::ShaderPackageCache,
	        .Status = snapshot.Pipeline.Status,
	        .Unit = ERendererDiagnosticUnit::Count,
	        .IntegerValue = snapshot.Pipeline.LazyRuntimeCount});

	snapshot.Metrics.push_back(
	    RendererDiagnosticMetric{
	        .Name = "shaderPackage.lastLoadTime",
	        .Origin = ERendererDiagnosticOrigin::ShaderPackageCache,
	        .Status = snapshot.Pipeline.LastShaderPackageLoad.PackageKey != 0u ? ERendererDiagnosticStatus::Available :
	                                                                              ERendererDiagnosticStatus::Unavailable,
	        .Unit = ERendererDiagnosticUnit::Microseconds,
	        .IntegerValue = snapshot.Pipeline.LastShaderPackageLoad.ElapsedMicroseconds,
	        .Detail = snapshot.Pipeline.LastShaderPackageLoad.PackagePath.string()});

	snapshot.Metrics.push_back(
	    RendererDiagnosticMetric{
	        .Name = "upload.usedBytes",
	        .Origin = ERendererDiagnosticOrigin::RhiAllocator,
	        .Status = snapshot.UploadPressure.Status,
	        .Unit = ERendererDiagnosticUnit::Bytes,
	        .IntegerValue = snapshot.UploadPressure.UsedBytes,
	        .Detail = snapshot.UploadPressure.Reason});

	snapshot.Metrics.push_back(
	    RendererDiagnosticMetric{
	        .Name = "upload.pressureRatio",
	        .Origin = ERendererDiagnosticOrigin::RhiAllocator,
	        .Status = snapshot.UploadPressure.Status,
	        .Unit = ERendererDiagnosticUnit::Ratio,
	        .NumericValue = snapshot.UploadPressure.BudgetUsageRatio,
	        .TextValue = RendererMemoryPressureLevelToString(snapshot.UploadPressure.Pressure),
	        .Detail = snapshot.UploadPressure.Reason});

	return snapshot;
}
