#include "PCH.h"

#include "Validation/RhiSmokeCaptureArtifacts.h"

#include "RHI/Public/Core/RhiBackendSelection.h"
#include "RHI/Public/RayTracing/RhiRayTracingDesc.h"

#include <fstream>
#include <string>

namespace RhiSmokeCaptureArtifacts
{
	std::filesystem::path DeriveMetadataPath(const RhiSmokeCaptureArtifactRequest& request)
	{
		if (!request.MetadataPath.empty())
		{
			return request.MetadataPath;
		}
		std::filesystem::path path = request.CapturePath;
		path += ".json";
		return path;
	}

	std::filesystem::path DeriveTimingCsvPath(const RhiSmokeCaptureArtifactRequest& request)
	{
		if (!request.TimingCsvPath.empty())
		{
			return request.TimingCsvPath;
		}
		std::filesystem::path path = request.CapturePath;
		path += ".timing.csv";
		return path;
	}

	std::string EscapeJson(std::string_view value)
	{
		std::string escaped;
		escaped.reserve(value.size());
		for (const char c : value)
		{
			switch (c)
			{
				case '\\':
					escaped += "\\\\";
					break;
				case '"':
					escaped += "\\\"";
					break;
				case '\n':
					escaped += "\\n";
					break;
				case '\r':
					escaped += "\\r";
					break;
				case '\t':
					escaped += "\\t";
					break;
				default:
					escaped += c;
					break;
			}
		}
		return escaped;
	}

	std::string EscapeCsv(std::string_view value)
	{
		bool requiresQuotes = false;
		for (const char character : value)
		{
			if (character == '"' || character == ',' || character == '\n' || character == '\r')
			{
				requiresQuotes = true;
				break;
			}
		}

		if (!requiresQuotes)
		{
			return std::string(value);
		}

		std::string escaped;
		escaped.reserve(value.size() + 2);
		escaped.push_back('"');
		for (const char character : value)
		{
			if (character == '"')
			{
				escaped += "\"\"";
				continue;
			}

			escaped.push_back(character);
		}
		escaped.push_back('"');
		return escaped;
	}

	void EnsureParentDirectory(const std::filesystem::path& path)
	{
		const std::filesystem::path parentPath = path.parent_path();
		if (!parentPath.empty())
		{
			std::filesystem::create_directories(parentPath);
		}
	}

	void WriteJson(const RhiSmokeCaptureArtifactRequest& request, const std::filesystem::path& metadataPath)
	{
		const RendererSmokeDiagnosticsSnapshot& diagnostics = request.Diagnostics;
		const RendererSmokeRayTracingDiagnostics& rayTracing = diagnostics.RayTracing;
		EnsureParentDirectory(metadataPath);

		std::ofstream file(metadataPath, std::ios::out | std::ios::trunc);
		if (!file)
		{
			return;
		}

		file << "{\n";
		file << "  \"capture\": {\n";
		file << "    \"path\": \"" << EscapeJson(request.CapturePath.string()) << "\",\n";
		file << "    \"success\": " << (request.CaptureResult ? "true" : "false") << ",\n";
		file << "    \"failureReason\": \"" << EscapeJson(request.CaptureResult.FailureReason) << "\",\n";
		file << "    \"frameIndex\": " << request.CaptureResult.FrameIndex << ",\n";
		file << "    \"viewMode\": " << request.CaptureResult.ViewMode << ",\n";
		file << "    \"viewModeName\": \"" << EscapeJson(request.CaptureResult.ViewModeName) << "\",\n";
		file << "    \"captureLabel\": \"" << EscapeJson(request.CaptureLabel) << "\",\n";
		file << "    \"purpose\": \"" << EscapeJson(request.CapturePurpose) << "\"\n";
		file << "  },\n";
		file << "  \"renderer\": {\n";
		file << "    \"backend\": \"" << RhiBackendApiToString(diagnostics.BackendApi) << "\",\n";
		file << "    \"adapter\": {\n";
		file << "      \"name\": \"" << EscapeJson(diagnostics.Adapter.Name) << "\",\n";
		file << "      \"driverDescription\": \"" << EscapeJson(diagnostics.Adapter.DriverDescription) << "\",\n";
		file << "      \"vendorId\": " << diagnostics.Adapter.VendorId << ",\n";
		file << "      \"deviceId\": " << diagnostics.Adapter.DeviceId << "\n";
		file << "    },\n";
		file << "    \"frameGraphUnresolvedBarrierWarnings\": " << diagnostics.FrameGraph.UnresolvedBarrierWarnings << ",\n";
		file << "    \"frameGraphMissingExecutionBindings\": " << diagnostics.FrameGraph.MissingExecutionBindings << ",\n";
		file << "    \"frameGraphTransientResources\": " << diagnostics.FrameGraph.TransientResources << ",\n";
		file << "    \"frameGraphImportedResources\": " << diagnostics.FrameGraph.ImportedResources << ",\n";
		file << "    \"frameGraphPersistentResources\": " << diagnostics.FrameGraph.PersistentResources << ",\n";
		file << "    \"frameGraphViewportProducts\": " << diagnostics.FrameGraph.ViewportProducts << ",\n";
		file << "    \"finalFrameGpuAvailable\": " << (diagnostics.FrameTimings.HasFinalFrameGpuMilliseconds ? "true" : "false") << ",\n";
		file << "    \"finalFrameGpuMs\": " << diagnostics.FrameTimings.FinalFrameGpuMilliseconds << ",\n";
		file << "    \"upscalerProvider\": \"" << EscapeJson(diagnostics.Upscaler.Provider) << "\",\n";
		file << "    \"upscalerStatus\": \"" << EscapeJson(diagnostics.Upscaler.Status) << "\",\n";
		file << "    \"upscalerReason\": \"" << EscapeJson(diagnostics.Upscaler.Reason) << "\"\n";
		file << "  },\n";
		file << "  \"rayTracing\": {\n";
		file << "    \"supported\": " << (rayTracing.Capability.Supported ? "true" : "false") << ",\n";
		file << "    \"inlineRayQuerySupported\": " << (rayTracing.Capability.InlineRayQuerySupported ? "true" : "false") << ",\n";
		file << "    \"topLevelProvider\": \"" << RhiRayTracingTopLevelProviderToString(rayTracing.Capability.TopLevelProvider) << "\",\n";
		file << "    \"topLevelProviderReason\": \"" << EscapeJson(rayTracing.Capability.TopLevelProviderReason) << "\",\n";
		file << "    \"ptlasProvider\": \"" << RhiPartitionedTlasProviderToString(rayTracing.PtlasPlanner.Provider) << "\",\n";
		file << "    \"ptlasSupported\": " << (rayTracing.PtlasPlanner.Supported ? "true" : "false") << ",\n";
		file << "    \"ptlasCapabilityReason\": \"" << EscapeJson(rayTracing.Capability.PartitionedTlasCapabilityReason) << "\",\n";
		file << "    \"classicTlasValid\": " << (rayTracing.ClassicTlas.Valid ? "true" : "false") << ",\n";
		file << "    \"classicTlasBuilt\": " << (rayTracing.ClassicTlas.Built ? "true" : "false") << ",\n";
		file << "    \"classicTlasInstances\": " << rayTracing.ClassicTlas.InstanceCount << ",\n";
		file << "    \"totalRenderInstances\": " << rayTracing.PtlasPlanner.TotalRenderInstanceCount << ",\n";
		file << "    \"traceableInstances\": " << rayTracing.PtlasPlanner.TraceableInstanceCount << ",\n";
		file << "    \"staticTraceableInstances\": " << rayTracing.PtlasPlanner.StaticTraceableInstanceCount << ",\n";
		file << "    \"dynamicTraceableInstances\": " << rayTracing.PtlasPlanner.DynamicTraceableInstanceCount << ",\n";
		file << "    \"partitionsPerAxis\": " << rayTracing.PtlasPlanner.PartitionsPerAxis << ",\n";
		file << "    \"partitions\": " << rayTracing.PtlasPlanner.PartitionCount << ",\n";
		file << "    \"gridPartitions\": " << rayTracing.PtlasPlanner.GridPartitionCount << ",\n";
		file << "    \"dirtyTransforms\": " << rayTracing.PtlasPlanner.DirtyTransformCount << ",\n";
		file << "    \"movedPartitions\": " << rayTracing.PtlasPlanner.MovedPartitionCount << ",\n";
		file << "    \"globalPartitionEligibleInstances\": " << rayTracing.PtlasPlanner.GlobalPartitionEligibleCount << ",\n";
		file << "    \"globalPartitionInstances\": " << rayTracing.PtlasPlanner.GlobalPartitionInstanceCount << ",\n";
		file << "    \"duplicateStableIndices\": " << rayTracing.PtlasPlanner.DuplicateStableIndexCount << ",\n";
		file << "    \"partitionOverflow\": " << (rayTracing.PtlasPlanner.Overflow ? "true" : "false") << ",\n";
		file << "    \"nativeOperations\": " << rayTracing.PtlasGpuUpdates.NativeOperationCount << ",\n";
		file << "    \"logicalUpdates\": " << rayTracing.PtlasGpuUpdates.LogicalUpdateCount << ",\n";
		file << "    \"validationMismatches\": " << rayTracing.PtlasGpuUpdates.ValidationMismatchCount << ",\n";
		file << "    \"requestedOperationWriterPath\": \""
		     << RhiPartitionedTlasOperationWriterPathToString(rayTracing.PtlasGpuUpdates.RequestedWriterPath) << "\",\n";
		file << "    \"operationWriterPath\": \""
		     << RhiPartitionedTlasOperationWriterPathToString(rayTracing.PtlasGpuUpdates.SelectedWriterPath) << "\",\n";
		file << "    \"operationWriterReason\": \"" << EscapeJson(rayTracing.PtlasGpuUpdates.WriterSelectionReason) << "\",\n";
		file << "    \"gpuDrivenOperationApiSupported\": "
		     << (rayTracing.PtlasGpuUpdates.GpuDrivenOperationApiSupported ? "true" : "false") << ",\n";
		file << "    \"gpuLogicalUpdateWriterAvailable\": "
		     << (rayTracing.PtlasGpuUpdates.GpuLogicalUpdateWriterAvailable ? "true" : "false") << ",\n";
		file << "    \"gpuNativePackAvailable\": " << (rayTracing.PtlasGpuUpdates.FullGpuNativePackAvailable ? "true" : "false") << ",\n";
		file << "    \"gpuNativePackSubmitted\": " << (rayTracing.PtlasGpuUpdates.FullGpuNativePackSubmitted ? "true" : "false") << "\n";
		file << "  },\n";
		file << "  \"gpuTimings\": [\n";
		for (std::size_t timingIndex = 0; timingIndex < diagnostics.FrameTimings.GpuTimings.size(); ++timingIndex)
		{
			const RendererGpuTimingMetric& timing = diagnostics.FrameTimings.GpuTimings[timingIndex];
			file << "    {\"label\": \"" << EscapeJson(timing.Label) << "\", \"depth\": " << timing.Depth
			     << ", \"durationMs\": " << timing.DurationMilliseconds << ", \"durationTicks\": " << timing.DurationTicks << "}";
			if (timingIndex + 1u < diagnostics.FrameTimings.GpuTimings.size())
			{
				file << ',';
			}
			file << '\n';
		}
		file << "  ]\n";
		file << "}\n";
	}

	void WriteTimingCsv(const RhiSmokeCaptureArtifactRequest& request, const std::filesystem::path& timingCsvPath)
	{
		const RendererSmokeRayTracingDiagnostics& rayTracing = request.Diagnostics.RayTracing;
		EnsureParentDirectory(timingCsvPath);

		std::ofstream file(timingCsvPath, std::ios::out | std::ios::trunc);
		if (!file)
		{
			return;
		}

		file << "backend,adapterName,vendorId,deviceId,driverDescription,viewMode,topLevelProvider,topLevelProviderReason,"
		        "ptlasProvider,ptlasSupported,ptlasCapabilityReason,classicTlasValid,classicTlasBuilt,totalRenderInstances,"
		        "traceableInstances,staticTraceableInstances,dynamicTraceableInstances,partitionsPerAxis,partitionCount,"
		        "gridPartitionCount,dirtyTransforms,dirtyRatio,movedPartitions,globalPartitionEligibleInstances,"
		        "globalPartitionInstances,duplicateStableIndices,partitionOverflow,requestedWriterPath,selectedWriterPath,"
		        "writerReason,logicalUpdates,nativeOperations,validationMismatches,gpuDrivenApiSupported,"
		        "gpuLogicalWriterAvailable,gpuNativePackAvailable,gpuNativePackSubmitted,finalFrameGpuAvailable,finalFrameGpuMs,"
		        "timingLabel,timingDepth,timingDurationMs,timingDurationTicks\n";
		const double dirtyRatio =
		    rayTracing.PtlasPlanner.TraceableInstanceCount > 0
		        ? static_cast<double>(rayTracing.PtlasPlanner.DirtyTransformCount) /
		              static_cast<double>(rayTracing.PtlasPlanner.TraceableInstanceCount)
		        : 0.0;
		file << EscapeCsv(RhiBackendApiToString(request.Diagnostics.BackendApi)) << ',';
		file << EscapeCsv(request.Diagnostics.Adapter.Name) << ',';
		file << request.Diagnostics.Adapter.VendorId << ',';
		file << request.Diagnostics.Adapter.DeviceId << ',';
		file << EscapeCsv(request.Diagnostics.Adapter.DriverDescription) << ',';
		file << EscapeCsv(request.CaptureResult.ViewModeName) << ',';
		file << EscapeCsv(RhiRayTracingTopLevelProviderToString(rayTracing.Capability.TopLevelProvider)) << ',';
		file << EscapeCsv(rayTracing.Capability.TopLevelProviderReason) << ',';
		file << EscapeCsv(RhiPartitionedTlasProviderToString(rayTracing.PtlasPlanner.Provider)) << ',';
		file << (rayTracing.PtlasPlanner.Supported ? "true" : "false") << ',';
		file << EscapeCsv(rayTracing.Capability.PartitionedTlasCapabilityReason) << ',';
		file << (rayTracing.ClassicTlas.Valid ? "true" : "false") << ',';
		file << (rayTracing.ClassicTlas.Built ? "true" : "false") << ',';
		file << rayTracing.PtlasPlanner.TotalRenderInstanceCount << ',';
		file << rayTracing.PtlasPlanner.TraceableInstanceCount << ',';
		file << rayTracing.PtlasPlanner.StaticTraceableInstanceCount << ',';
		file << rayTracing.PtlasPlanner.DynamicTraceableInstanceCount << ',';
		file << rayTracing.PtlasPlanner.PartitionsPerAxis << ',';
		file << rayTracing.PtlasPlanner.PartitionCount << ',';
		file << rayTracing.PtlasPlanner.GridPartitionCount << ',';
		file << rayTracing.PtlasPlanner.DirtyTransformCount << ',';
		file << dirtyRatio << ',';
		file << rayTracing.PtlasPlanner.MovedPartitionCount << ',';
		file << rayTracing.PtlasPlanner.GlobalPartitionEligibleCount << ',';
		file << rayTracing.PtlasPlanner.GlobalPartitionInstanceCount << ',';
		file << rayTracing.PtlasPlanner.DuplicateStableIndexCount << ',';
		file << (rayTracing.PtlasPlanner.Overflow ? "true" : "false") << ',';
		file << EscapeCsv(RhiPartitionedTlasOperationWriterPathToString(rayTracing.PtlasGpuUpdates.RequestedWriterPath)) << ',';
		file << EscapeCsv(RhiPartitionedTlasOperationWriterPathToString(rayTracing.PtlasGpuUpdates.SelectedWriterPath)) << ',';
		file << EscapeCsv(rayTracing.PtlasGpuUpdates.WriterSelectionReason) << ',';
		file << rayTracing.PtlasGpuUpdates.LogicalUpdateCount << ',';
		file << rayTracing.PtlasGpuUpdates.NativeOperationCount << ',';
		file << rayTracing.PtlasGpuUpdates.ValidationMismatchCount << ',';
		file << (rayTracing.PtlasGpuUpdates.GpuDrivenOperationApiSupported ? "true" : "false") << ',';
		file << (rayTracing.PtlasGpuUpdates.GpuLogicalUpdateWriterAvailable ? "true" : "false") << ',';
		file << (rayTracing.PtlasGpuUpdates.FullGpuNativePackAvailable ? "true" : "false") << ',';
		file << (rayTracing.PtlasGpuUpdates.FullGpuNativePackSubmitted ? "true" : "false") << ',';
		file << (request.Diagnostics.FrameTimings.HasFinalFrameGpuMilliseconds ? "true" : "false") << ',';
		file << request.Diagnostics.FrameTimings.FinalFrameGpuMilliseconds;
		if (request.Diagnostics.FrameTimings.GpuTimings.empty())
		{
			file << ",,,\n";
			return;
		}

		for (std::size_t timingIndex = 0; timingIndex < request.Diagnostics.FrameTimings.GpuTimings.size(); ++timingIndex)
		{
			const RendererGpuTimingMetric& timing = request.Diagnostics.FrameTimings.GpuTimings[timingIndex];
			if (timingIndex > 0)
			{
				file << EscapeCsv(RhiBackendApiToString(request.Diagnostics.BackendApi)) << ',';
				file << EscapeCsv(request.Diagnostics.Adapter.Name) << ',';
				file << request.Diagnostics.Adapter.VendorId << ',';
				file << request.Diagnostics.Adapter.DeviceId << ',';
				file << EscapeCsv(request.Diagnostics.Adapter.DriverDescription) << ',';
				file << EscapeCsv(request.CaptureResult.ViewModeName) << ',';
				file << EscapeCsv(RhiRayTracingTopLevelProviderToString(rayTracing.Capability.TopLevelProvider)) << ',';
				file << EscapeCsv(rayTracing.Capability.TopLevelProviderReason) << ',';
				file << EscapeCsv(RhiPartitionedTlasProviderToString(rayTracing.PtlasPlanner.Provider)) << ',';
				file << (rayTracing.PtlasPlanner.Supported ? "true" : "false") << ',';
				file << EscapeCsv(rayTracing.Capability.PartitionedTlasCapabilityReason) << ',';
				file << (rayTracing.ClassicTlas.Valid ? "true" : "false") << ',';
				file << (rayTracing.ClassicTlas.Built ? "true" : "false") << ',';
				file << rayTracing.PtlasPlanner.TotalRenderInstanceCount << ',';
				file << rayTracing.PtlasPlanner.TraceableInstanceCount << ',';
				file << rayTracing.PtlasPlanner.StaticTraceableInstanceCount << ',';
				file << rayTracing.PtlasPlanner.DynamicTraceableInstanceCount << ',';
				file << rayTracing.PtlasPlanner.PartitionsPerAxis << ',';
				file << rayTracing.PtlasPlanner.PartitionCount << ',';
				file << rayTracing.PtlasPlanner.GridPartitionCount << ',';
				file << rayTracing.PtlasPlanner.DirtyTransformCount << ',';
				file << dirtyRatio << ',';
				file << rayTracing.PtlasPlanner.MovedPartitionCount << ',';
				file << rayTracing.PtlasPlanner.GlobalPartitionEligibleCount << ',';
				file << rayTracing.PtlasPlanner.GlobalPartitionInstanceCount << ',';
				file << rayTracing.PtlasPlanner.DuplicateStableIndexCount << ',';
				file << (rayTracing.PtlasPlanner.Overflow ? "true" : "false") << ',';
				file << EscapeCsv(RhiPartitionedTlasOperationWriterPathToString(rayTracing.PtlasGpuUpdates.RequestedWriterPath)) << ',';
				file << EscapeCsv(RhiPartitionedTlasOperationWriterPathToString(rayTracing.PtlasGpuUpdates.SelectedWriterPath)) << ',';
				file << EscapeCsv(rayTracing.PtlasGpuUpdates.WriterSelectionReason) << ',';
				file << rayTracing.PtlasGpuUpdates.LogicalUpdateCount << ',';
				file << rayTracing.PtlasGpuUpdates.NativeOperationCount << ',';
				file << rayTracing.PtlasGpuUpdates.ValidationMismatchCount << ',';
				file << (rayTracing.PtlasGpuUpdates.GpuDrivenOperationApiSupported ? "true" : "false") << ',';
				file << (rayTracing.PtlasGpuUpdates.GpuLogicalUpdateWriterAvailable ? "true" : "false") << ',';
				file << (rayTracing.PtlasGpuUpdates.FullGpuNativePackAvailable ? "true" : "false") << ',';
				file << (rayTracing.PtlasGpuUpdates.FullGpuNativePackSubmitted ? "true" : "false") << ',';
				file << (request.Diagnostics.FrameTimings.HasFinalFrameGpuMilliseconds ? "true" : "false") << ',';
				file << request.Diagnostics.FrameTimings.FinalFrameGpuMilliseconds;
			}
			file << ',' << EscapeCsv(timing.Label) << ',' << timing.Depth << ',' << timing.DurationMilliseconds << ','
			     << timing.DurationTicks << '\n';
		}
	}

	void Write(const RhiSmokeCaptureArtifactRequest& request) noexcept
	{
		try
		{
			if (request.CapturePath.empty())
			{
				return;
			}
			WriteJson(request, DeriveMetadataPath(request));
			WriteTimingCsv(request, DeriveTimingCsvPath(request));
		}
		catch (...)
		{
		}
	}
}
