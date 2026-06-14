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
		file << "    \"viewModeName\": \"" << EscapeJson(request.CaptureResult.ViewModeName) << "\"\n";
		file << "  },\n";
		file << "  \"renderer\": {\n";
		file << "    \"backend\": \"" << RhiBackendApiToString(diagnostics.BackendApi) << "\",\n";
		file << "    \"frameGraphUnresolvedBarrierWarnings\": " << diagnostics.FrameGraph.UnresolvedBarrierWarnings << ",\n";
		file << "    \"upscalerProvider\": \"" << EscapeJson(diagnostics.Upscaler.Provider) << "\",\n";
		file << "    \"upscalerStatus\": \"" << EscapeJson(diagnostics.Upscaler.Status) << "\",\n";
		file << "    \"upscalerReason\": \"" << EscapeJson(diagnostics.Upscaler.Reason) << "\"\n";
		file << "  },\n";
		file << "  \"rayTracing\": {\n";
		file << "    \"supported\": " << (rayTracing.Capability.Supported ? "true" : "false") << ",\n";
		file << "    \"inlineRayQuerySupported\": " << (rayTracing.Capability.InlineRayQuerySupported ? "true" : "false") << ",\n";
		file << "    \"topLevelProvider\": \"" << RhiRayTracingTopLevelProviderToString(rayTracing.Capability.TopLevelProvider) << "\",\n";
		file << "    \"ptlasProvider\": \"" << RhiPartitionedTlasProviderToString(rayTracing.PtlasPlanner.Provider) << "\",\n";
		file << "    \"ptlasSupported\": " << (rayTracing.PtlasPlanner.Supported ? "true" : "false") << ",\n";
		file << "    \"classicTlasValid\": " << (rayTracing.ClassicTlas.Valid ? "true" : "false") << ",\n";
		file << "    \"classicTlasInstances\": " << rayTracing.ClassicTlas.InstanceCount << ",\n";
		file << "    \"partitions\": " << rayTracing.PtlasPlanner.PartitionCount << ",\n";
		file << "    \"dirtyTransforms\": " << rayTracing.PtlasPlanner.DirtyTransformCount << ",\n";
		file << "    \"movedPartitions\": " << rayTracing.PtlasPlanner.MovedPartitionCount << ",\n";
		file << "    \"globalPartitionInstances\": " << rayTracing.PtlasPlanner.GlobalPartitionInstanceCount << ",\n";
		file << "    \"nativeOperations\": " << rayTracing.PtlasGpuUpdates.NativeOperationCount << ",\n";
		file << "    \"logicalUpdates\": " << rayTracing.PtlasGpuUpdates.LogicalUpdateCount << ",\n";
		file << "    \"requestedOperationWriterPath\": \""
		     << RhiPartitionedTlasOperationWriterPathToString(rayTracing.PtlasGpuUpdates.RequestedWriterPath) << "\",\n";
		file << "    \"operationWriterPath\": \""
		     << RhiPartitionedTlasOperationWriterPathToString(rayTracing.PtlasGpuUpdates.SelectedWriterPath) << "\",\n";
		file << "    \"operationWriterReason\": \"" << EscapeJson(rayTracing.PtlasGpuUpdates.WriterSelectionReason) << "\",\n";
		file << "    \"gpuNativePackSupported\": " << (rayTracing.PtlasGpuUpdates.FullGpuNativePackSupported ? "true" : "false") << ",\n";
		file << "    \"gpuNativePackSubmitted\": " << (rayTracing.PtlasGpuUpdates.FullGpuNativePackSubmitted ? "true" : "false") << ",\n";
		file << "    \"timingsMs\": {\n";
		file << "      \"scenePrepareCpu\": " << rayTracing.FrameTimings.ScenePrepareCpuMilliseconds << ",\n";
		file << "      \"sceneBuildCpu\": " << rayTracing.FrameTimings.SceneBuildCpuMilliseconds << ",\n";
		file << "      \"blasCpu\": " << rayTracing.Blas.CpuMilliseconds << ",\n";
		file << "      \"blasGpu\": " << rayTracing.Blas.GpuMilliseconds << ",\n";
		file << "      \"classicTlasCpu\": " << rayTracing.ClassicTlas.CpuMilliseconds << ",\n";
		file << "      \"classicTlasGpu\": " << rayTracing.ClassicTlas.GpuMilliseconds << ",\n";
		file << "      \"rayTracingPassGpu\": " << rayTracing.FrameTimings.RayTracingPassGpuMilliseconds << "\n";
		file << "    }\n";
		file << "  }\n";
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

		file << "backend,viewMode,topLevelProvider,ptlasProvider,requestedWriterPath,selectedWriterPath,writerReason,"
		        "scenePrepareCpuMs,sceneBuildCpuMs,blasCpuMs,blasGpuMs,classicTlasCpuMs,classicTlasGpuMs,rayTracingPassGpuMs,"
		        "logicalUpdates,nativeOperations,cpuPackMs,gpuDirtyMs,gpuNativePackMs,ptlasUpdateGpuMs\n";
		file << RhiBackendApiToString(request.Diagnostics.BackendApi) << ',';
		file << request.CaptureResult.ViewModeName << ',';
		file << RhiRayTracingTopLevelProviderToString(rayTracing.Capability.TopLevelProvider) << ',';
		file << RhiPartitionedTlasProviderToString(rayTracing.PtlasPlanner.Provider) << ',';
		file << RhiPartitionedTlasOperationWriterPathToString(rayTracing.PtlasGpuUpdates.RequestedWriterPath) << ',';
		file << RhiPartitionedTlasOperationWriterPathToString(rayTracing.PtlasGpuUpdates.SelectedWriterPath) << ',';
		file << rayTracing.PtlasGpuUpdates.WriterSelectionReason << ',';
		file << rayTracing.FrameTimings.ScenePrepareCpuMilliseconds << ',';
		file << rayTracing.FrameTimings.SceneBuildCpuMilliseconds << ',';
		file << rayTracing.Blas.CpuMilliseconds << ',';
		file << rayTracing.Blas.GpuMilliseconds << ',';
		file << rayTracing.ClassicTlas.CpuMilliseconds << ',';
		file << rayTracing.ClassicTlas.GpuMilliseconds << ',';
		file << rayTracing.FrameTimings.RayTracingPassGpuMilliseconds << ',';
		file << rayTracing.PtlasGpuUpdates.LogicalUpdateCount << ',';
		file << rayTracing.PtlasGpuUpdates.NativeOperationCount << ',';
		file << rayTracing.PtlasGpuUpdates.CpuPackMilliseconds << ',';
		file << rayTracing.PtlasGpuUpdates.GpuDirtyDetectionMilliseconds << ',';
		file << rayTracing.PtlasGpuUpdates.GpuNativePackMilliseconds << ',';
		file << rayTracing.PtlasGpuUpdates.PtlasUpdateGpuMilliseconds << '\n';
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
