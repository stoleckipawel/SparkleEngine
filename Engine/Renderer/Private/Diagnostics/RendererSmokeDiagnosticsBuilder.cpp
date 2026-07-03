#include "PCH.h"

#include "Diagnostics/RendererSmokeDiagnosticsBuilder.h"

#include "Diagnostics/RendererSmokeRayTracingSnapshotBuilder.h"
#include "FramePipeline/FramePipeline.h"
#include "Host/RendererSystemRoot.h"
#include "RHI/Public/Core/RhiCapabilities.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"

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
		snapshot.FrameTimings.GpuTimings = frameTimings.GpuTimings;
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
	return snapshot;
}
