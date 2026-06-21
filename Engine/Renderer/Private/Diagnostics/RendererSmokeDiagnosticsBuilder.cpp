#include "PCH.h"

#include "Diagnostics/RendererSmokeDiagnosticsBuilder.h"

#include "Diagnostics/RendererSmokeRayTracingSnapshotBuilder.h"
#include "FramePipeline/FramePipeline.h"
#include "Host/RendererSystemRoot.h"
#include "RHI/Public/Core/RhiCapabilities.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "Upscaling/UpscalerProvider.h"
#include "Upscaling/UpscalerSubsystem.h"

namespace
{
	void FillFrameGraphDiagnostics(RendererSmokeDiagnosticsSnapshot& snapshot, const FramePipeline* framePipeline) noexcept
	{
		if (framePipeline == nullptr)
		{
			return;
		}

		snapshot.FrameGraph.UnresolvedBarrierWarnings = framePipeline->GetLastUnresolvedBarrierWarningCount();
		snapshot.FrameGraph.MissingExecutionBindings = framePipeline->GetLastMissingExecutionBindingCount();
		snapshot.FrameGraph.TransientResources = framePipeline->GetCompiledTransientResourceCount();
		snapshot.FrameGraph.ImportedResources = framePipeline->GetCompiledImportedResourceCount();
		snapshot.FrameGraph.PersistentResources = framePipeline->GetCompiledPersistentResourceCount();
		snapshot.FrameGraph.ViewportProducts = framePipeline->GetAvailableViewportProductCount();
	}

	void FillFrameTimingDiagnostics(RendererSmokeDiagnosticsSnapshot& snapshot, const FramePipeline* framePipeline)
	{
		if (framePipeline == nullptr)
		{
			return;
		}

		const RendererFrameTimingDiagnosticsSnapshot frameTimings = framePipeline->CaptureFrameTimingDiagnosticsSnapshot();
		double finalFrameGpuMilliseconds = 0.0;
		snapshot.FrameTimings.HasFinalFrameGpuMilliseconds =
		    TryGetRendererGpuTimingMilliseconds(frameTimings, "GPU Frame", finalFrameGpuMilliseconds);
		snapshot.FrameTimings.FinalFrameGpuMilliseconds = finalFrameGpuMilliseconds;
		snapshot.FrameTimings.GpuTimings = frameTimings.GpuTimings;
	}

	void FillUpscalerDiagnostics(RendererSmokeDiagnosticsSnapshot& snapshot, const RendererSystemRoot& systems)
	{
		const UpscalerSubsystem* upscalerSubsystem = systems.GetUpscalerSubsystem();
		if (upscalerSubsystem == nullptr)
		{
			return;
		}

		const UpscalerProviderCapabilities upscalerDiagnostics = upscalerSubsystem->GetDiagnostics();
		snapshot.Upscaler.Provider = upscalerDiagnostics.ProviderName;
		snapshot.Upscaler.Status = RendererProviderCapabilityStateToString(upscalerDiagnostics.CapabilityState);
		snapshot.Upscaler.Reason = upscalerDiagnostics.Reason;
	}
}

RendererSmokeDiagnosticsSnapshot RendererSmokeDiagnosticsBuilder::Build(
    const RendererSystemRoot& systems,
    const FramePipeline* framePipeline)
{
	RendererSmokeDiagnosticsSnapshot snapshot{};
	const RhiCapabilities capabilities = systems.GetRenderHardwareInterface().GetCapabilities();
	snapshot.BackendApi = capabilities.BackendApi;
	snapshot.Adapter.Name = capabilities.ExternalFeatureInterop.Adapter.Name;
	snapshot.Adapter.DriverDescription = capabilities.ExternalFeatureInterop.Adapter.DriverDescription;
	snapshot.Adapter.VendorId = capabilities.ExternalFeatureInterop.Adapter.VendorId;
	snapshot.Adapter.DeviceId = capabilities.ExternalFeatureInterop.Adapter.DeviceId;

	FillFrameGraphDiagnostics(snapshot, framePipeline);
	FillFrameTimingDiagnostics(snapshot, framePipeline);
	snapshot.RayTracing =
	    RendererSmokeRayTracingSnapshotBuilder::Build(capabilities.RayTracing, systems.GetRenderRayTracingScene());
	FillUpscalerDiagnostics(snapshot, systems);
	return snapshot;
}
