#include "PCH.h"

#include "Renderer/Public/Diagnostics/RendererDiagnosticsSnapshot.h"

#include <filesystem>
#include <fstream>
#include <system_error>

namespace
{
	const char* BoolToString(bool value) noexcept
	{
		return value ? "true" : "false";
	}

	void WriteMetric(std::ostream& stream, const RendererDiagnosticMetric& metric)
	{
		stream << "- " << metric.Name << ": status=" << RendererDiagnosticStatusToString(metric.Status)
		       << ", origin=" << RendererDiagnosticOriginToString(metric.Origin)
		       << ", unit=" << RendererDiagnosticUnitToString(metric.Unit);
		if (!metric.TextValue.empty())
		{
			stream << ", value=" << metric.TextValue;
		}
		else if (metric.Unit == ERendererDiagnosticUnit::Ratio || metric.Unit == ERendererDiagnosticUnit::Milliseconds)
		{
			stream << ", value=" << metric.NumericValue;
		}
		else
		{
			stream << ", value=" << metric.IntegerValue;
		}
		if (!metric.Detail.empty())
		{
			stream << ", detail=" << metric.Detail;
		}
		stream << '\n';
	}
}

const char* RendererDiagnosticStatusToString(ERendererDiagnosticStatus status) noexcept
{
	switch (status)
	{
		case ERendererDiagnosticStatus::Available:
			return "Available";
		case ERendererDiagnosticStatus::Unavailable:
			return "Unavailable";
		case ERendererDiagnosticStatus::Unsupported:
			return "Unsupported";
		case ERendererDiagnosticStatus::Planned:
			return "Planned";
	}

	return "Unknown";
}

const char* RendererDiagnosticUnitToString(ERendererDiagnosticUnit unit) noexcept
{
	switch (unit)
	{
		case ERendererDiagnosticUnit::None:
			return "None";
		case ERendererDiagnosticUnit::Boolean:
			return "Boolean";
		case ERendererDiagnosticUnit::Bytes:
			return "Bytes";
		case ERendererDiagnosticUnit::Count:
			return "Count";
		case ERendererDiagnosticUnit::Ratio:
			return "Ratio";
		case ERendererDiagnosticUnit::Milliseconds:
			return "Milliseconds";
		case ERendererDiagnosticUnit::Microseconds:
			return "Microseconds";
		case ERendererDiagnosticUnit::Ticks:
			return "Ticks";
		case ERendererDiagnosticUnit::Hertz:
			return "Hertz";
		case ERendererDiagnosticUnit::Text:
			return "Text";
	}

	return "Unknown";
}

const char* RendererDiagnosticOriginToString(ERendererDiagnosticOrigin origin) noexcept
{
	switch (origin)
	{
		case ERendererDiagnosticOrigin::RhiBackend:
			return "RHI backend";
		case ERendererDiagnosticOrigin::RhiAllocator:
			return "RHI allocator";
		case ERendererDiagnosticOrigin::RhiDescriptorService:
			return "RHI descriptor service";
		case ERendererDiagnosticOrigin::RendererFrame:
			return "Renderer frame";
		case ERendererDiagnosticOrigin::RendererProvider:
			return "Renderer provider";
		case ERendererDiagnosticOrigin::ShaderPackageCache:
			return "Shader package cache";
		case ERendererDiagnosticOrigin::CoreProfiler:
			return "Core profiler";
	}

	return "Unknown";
}

const char* RendererBackendApiToString(ERhiBackendApi backend) noexcept
{
	switch (backend)
	{
		case ERhiBackendApi::D3D12:
			return "D3D12";
		case ERhiBackendApi::Vulkan:
			return "Vulkan";
		case ERhiBackendApi::Unknown:
		default:
			return "Unknown";
	}
}

std::string FormatRendererBackendVersion(const RhiBackendVersionInfo& version)
{
	if (!version.IsKnown())
	{
		return "Unknown";
	}

	return std::string(RhiBackendVersionSemanticToString(version.Semantic)) + " " + std::to_string(version.Major) + "." +
	       std::to_string(version.Minor) + "." + std::to_string(version.Patch);
}

bool WriteRendererDiagnosticsTextArtifact(
    const RendererDiagnosticsSnapshot& snapshot,
    const std::filesystem::path& outputPath,
    std::string* outErrorMessage)
{
	std::error_code errorCode;
	if (outputPath.has_parent_path())
	{
		std::filesystem::create_directories(outputPath.parent_path(), errorCode);
		if (errorCode)
		{
			if (outErrorMessage != nullptr)
			{
				*outErrorMessage = "Failed to create diagnostics artifact directory: " + errorCode.message();
			}
			return false;
		}
	}

	std::ofstream stream(outputPath, std::ios::out | std::ios::trunc);
	if (!stream.is_open())
	{
		if (outErrorMessage != nullptr)
		{
			*outErrorMessage = "Failed to open diagnostics artifact for writing: " + outputPath.string();
		}
		return false;
	}

	stream << "# Renderer Diagnostics Snapshot\n\n";
	stream << "frameIndex=" << snapshot.FrameIndex << "\n\n";

	stream << "## Backend\n";
	stream << "status=" << RendererDiagnosticStatusToString(snapshot.Backend.Status) << '\n';
	stream << "backend=" << snapshot.Backend.BackendName << '\n';
	stream << "version=" << snapshot.Backend.VersionText << '\n';
	stream << "adapter=" << snapshot.Backend.Adapter.Name << '\n';
	stream << "validationEnabled=" << BoolToString(snapshot.Backend.DiagnosticsSupport.ValidationEnabled) << '\n';
	stream << "timestampQueries=" << BoolToString(snapshot.Backend.DiagnosticsSupport.SupportsTimestampQueries) << '\n';
	stream << "memoryAllocator=" << RhiMemoryAllocatorBackendToString(snapshot.Memory.MemoryUsage.AllocatorBackend) << "\n\n";

	stream << "## Provider\n";
	stream << "status=" << RendererDiagnosticStatusToString(snapshot.Provider.Status) << '\n';
	stream << "requested=" << snapshot.Provider.RequestedProvider << '\n';
	stream << "active=" << snapshot.Provider.ActiveProvider << '\n';
	stream << "category=" << snapshot.Provider.Category << '\n';
	stream << "capabilityState=" << snapshot.Provider.CapabilityState << '\n';
	stream << "canEvaluate=" << BoolToString(snapshot.Provider.CanEvaluate) << '\n';
	stream << "externalSdk=" << BoolToString(snapshot.Provider.UsesExternalSdk) << '\n';
	stream << "resourceContract=" << snapshot.Provider.ResourceContract << '\n';
	stream << "reason=" << snapshot.Provider.Reason << "\n\n";

	stream << "## Memory\n";
	stream << "available=" << BoolToString(snapshot.Memory.Available) << '\n';
	stream << "totalUsedBytes=" << snapshot.Memory.MemoryUsage.TotalUsedBytes << '\n';
	stream << "totalBudgetBytes=" << snapshot.Memory.MemoryUsage.TotalBudgetBytes << '\n';
	stream << "uploadUsedBytes=" << snapshot.UploadPressure.UsedBytes << '\n';
	stream << "uploadBudgetBytes=" << snapshot.UploadPressure.BudgetBytes << '\n';
	stream << "uploadPressure=" << RendererMemoryPressureLevelToString(snapshot.UploadPressure.Pressure) << "\n\n";

	stream << "## Descriptors\n";
	stream << "descriptorModel=" << RhiDescriptorModelToString(snapshot.Descriptors.DescriptorModel) << '\n';
	for (const RhiDescriptorAllocatorUsage& allocator : snapshot.Descriptors.Allocators)
	{
		stream << "- " << allocator.Name << ": status=" << RhiDescriptorUsageStatusToString(allocator.Status)
		       << ", capacity=" << allocator.Capacity << ", allocated=" << allocator.Allocated << ", free=" << allocator.Free
		       << ", highWatermark=" << allocator.HighWatermark << ", occupancy=" << allocator.OccupancyRatio;
		if (!allocator.Reason.empty())
		{
			stream << ", reason=" << allocator.Reason;
		}
		stream << '\n';
	}
	stream << '\n';

	stream << "## Pipeline\n";
	stream << "status=" << RendererDiagnosticStatusToString(snapshot.Pipeline.Status) << '\n';
	stream << "shaderPackageGeneration=" << snapshot.Pipeline.ShaderPackageGeneration << '\n';
	stream << "lazyRuntimeCount=" << snapshot.Pipeline.LazyRuntimeCount << '\n';
	stream << "lastShaderPackageLoadMicroseconds=" << snapshot.Pipeline.LastShaderPackageLoad.ElapsedMicroseconds << '\n';
	stream << "lastShaderPackageCacheHit=" << BoolToString(snapshot.Pipeline.LastShaderPackageLoad.WasCacheHit) << '\n';
	stream << "pipelineCacheStatus=" << RendererDiagnosticStatusToString(snapshot.Pipeline.PipelineCacheStatus) << '\n';
	stream << "pipelineCacheReason=" << snapshot.Pipeline.PipelineCacheReason << "\n\n";

	stream << "## Frame Timing\n";
	stream << "gpuTimingStatus=" << RendererDiagnosticStatusToString(snapshot.FrameTiming.GpuTimingStatus) << '\n';
	for (const RendererGpuTimingMetric& timing : snapshot.FrameTiming.GpuTimings)
	{
		stream << "- " << timing.Label << ": " << timing.DurationMilliseconds << " ms, ticks=" << timing.DurationTicks
		       << ", depth=" << timing.Depth << '\n';
	}
	stream << "cpuFrameTimingStatus=" << RendererDiagnosticStatusToString(snapshot.FrameTiming.CpuFrameTimingStatus) << '\n';
	stream << "cpuFrameTimingReason=" << snapshot.FrameTiming.CpuFrameTimingReason << "\n\n";

	stream << "## Metrics\n";
	for (const RendererDiagnosticMetric& metric : snapshot.Metrics)
	{
		WriteMetric(stream, metric);
	}

	return true;
}
