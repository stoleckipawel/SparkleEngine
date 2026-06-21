#pragma once

#include "RendererAPI.h"
#include "RendererMemoryDiagnostics.h"
#include "../../../RHI/Public/Core/RhiCapabilities.h"
#include "../../../RHI/Public/Core/RhiBackendApi.h"
#include "../../../RHI/Public/Descriptors/RhiDescriptorService.h"
#include "../../../RHI/Public/Diagnostics/RhiDiagnostics.h"
#include "../../../RHI/Public/Shaders/CookedShaderPackageCache.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

enum class ERendererDiagnosticStatus : std::uint8_t
{
	Available = 0,
	Unavailable = 1,
	Unsupported = 2,
	Planned = 3,
};

enum class ERendererDiagnosticUnit : std::uint8_t
{
	None = 0,
	Boolean,
	Bytes,
	Count,
	Ratio,
	Milliseconds,
	Microseconds,
	Ticks,
	Hertz,
	Text,
};

enum class ERendererDiagnosticOrigin : std::uint8_t
{
	RhiBackend = 0,
	RhiAllocator,
	RhiDescriptorService,
	RendererFrame,
	RendererProvider,
	ShaderPackageCache,
	CoreProfiler,
};

struct SPARKLE_RENDERER_API RendererDiagnosticMetric final
{
	std::string Name;
	ERendererDiagnosticOrigin Origin = ERendererDiagnosticOrigin::RendererFrame;
	ERendererDiagnosticStatus Status = ERendererDiagnosticStatus::Unavailable;
	ERendererDiagnosticUnit Unit = ERendererDiagnosticUnit::None;
	double NumericValue = 0.0;
	std::uint64_t IntegerValue = 0;
	std::string TextValue;
	std::string Detail;
};

struct SPARKLE_RENDERER_API RendererBackendDiagnosticsSnapshot final
{
	ERendererDiagnosticStatus Status = ERendererDiagnosticStatus::Unavailable;
	ERhiBackendApi BackendApi = ERhiBackendApi::Unknown;
	std::string BackendName;
	RhiBackendVersionInfo Version;
	std::string VersionText;
	RhiAdapterIdentity Adapter;
	RhiBackendDiagnosticsSupport DiagnosticsSupport;
	RhiBackendMemorySupport MemorySupport;
	RhiDiagnosticsCapabilities RuntimeDiagnostics;
};

struct SPARKLE_RENDERER_API RendererProviderDiagnosticsSnapshot final
{
	ERendererDiagnosticStatus Status = ERendererDiagnosticStatus::Unavailable;
	std::string RequestedProvider;
	std::string ActiveProvider;
	std::string Category;
	std::string CapabilityState;
	std::string FailureDomain;
	bool CanEvaluate = false;
	bool UsesExternalSdk = false;
	std::string RuntimeVersion;
	std::string RuntimeState;
	std::string ResourceContract;
	std::string Reason;
};

struct SPARKLE_RENDERER_API RendererPipelineDiagnosticsSnapshot final
{
	ERendererDiagnosticStatus Status = ERendererDiagnosticStatus::Unavailable;
	std::uint64_t ShaderPackageGeneration = 0;
	std::uint32_t LazyRuntimeCount = 0;
	CookedShaderPackageLoadReport LastShaderPackageLoad;
	ERendererDiagnosticStatus PipelineCacheStatus = ERendererDiagnosticStatus::Planned;
	std::string PipelineCacheReason;
};

struct SPARKLE_RENDERER_API RendererGpuTimingMetric final
{
	std::string Label;
	std::uint64_t BeginTicks = 0;
	std::uint64_t EndTicks = 0;
	std::uint64_t DurationTicks = 0;
	double DurationMilliseconds = 0.0;
	std::uint16_t Depth = 0;
};

struct SPARKLE_RENDERER_API RendererFrameTimingDiagnosticsSnapshot final
{
	ERendererDiagnosticStatus GpuTimingStatus = ERendererDiagnosticStatus::Unavailable;
	std::vector<RendererGpuTimingMetric> GpuTimings;
	ERendererDiagnosticStatus CpuFrameTimingStatus = ERendererDiagnosticStatus::Available;
	std::string CpuFrameTimingReason;
};

struct SPARKLE_RENDERER_API RendererUploadPressureDiagnosticsSnapshot final
{
	ERendererDiagnosticStatus Status = ERendererDiagnosticStatus::Unavailable;
	std::uint64_t UsedBytes = 0;
	std::uint64_t BudgetBytes = 0;
	float BudgetUsageRatio = 0.0f;
	RendererMemoryPressureLevel Pressure = RendererMemoryPressureLevel::Normal;
	std::string Reason;
};

struct SPARKLE_RENDERER_API RendererDiagnosticsSnapshot final
{
	std::uint64_t FrameIndex = 0;
	RendererBackendDiagnosticsSnapshot Backend;
	RendererProviderDiagnosticsSnapshot Provider;
	RendererMemoryDiagnosticsSnapshot Memory;
	RhiDescriptorUsageSnapshot Descriptors;
	RendererUploadPressureDiagnosticsSnapshot UploadPressure;
	RendererPipelineDiagnosticsSnapshot Pipeline;
	RendererFrameTimingDiagnosticsSnapshot FrameTiming;
	std::vector<RendererDiagnosticMetric> Metrics;
};

SPARKLE_RENDERER_API const char* RendererDiagnosticStatusToString(ERendererDiagnosticStatus status) noexcept;
SPARKLE_RENDERER_API const char* RendererDiagnosticUnitToString(ERendererDiagnosticUnit unit) noexcept;
SPARKLE_RENDERER_API const char* RendererDiagnosticOriginToString(ERendererDiagnosticOrigin origin) noexcept;
SPARKLE_RENDERER_API const char* RendererBackendApiToString(ERhiBackendApi backend) noexcept;
SPARKLE_RENDERER_API const RendererGpuTimingMetric* FindRendererGpuTiming(
    const RendererFrameTimingDiagnosticsSnapshot& snapshot,
    std::string_view label) noexcept;
SPARKLE_RENDERER_API bool TryGetRendererGpuTimingMilliseconds(
    const RendererFrameTimingDiagnosticsSnapshot& snapshot,
    std::string_view label,
    double& outMilliseconds) noexcept;
SPARKLE_RENDERER_API std::string FormatRendererBackendVersion(const RhiBackendVersionInfo& version);
SPARKLE_RENDERER_API bool WriteRendererDiagnosticsTextArtifact(
    const RendererDiagnosticsSnapshot& snapshot,
    const std::filesystem::path& outputPath,
    std::string* outErrorMessage = nullptr);
