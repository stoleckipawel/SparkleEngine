#pragma once

#include "../RendererAPI.h"
#include "../../../RHI/Public/Memory/RhiMemoryDiagnostics.h"

#include <cstdint>
#include <vector>

enum class RendererMemoryPressureLevel : std::uint8_t
{
	Normal,
	Watch,
	Pressure,
	Critical,
};

struct SPARKLE_RENDERER_API RendererMemoryPressureThresholds final
{
	float WatchRatio = 0.70f;
	float PressureRatio = 0.85f;
	float CriticalRatio = 0.95f;
};

struct SPARKLE_RENDERER_API RendererMemoryCategoryPressure final
{
	RhiMemoryCategory Category = RhiMemoryCategory::Other;
	RhiMemoryResidencyClass ResidencyClass = RhiMemoryResidencyClass::DeviceLocal;
	std::uint64_t UsedBytes = 0;
	std::uint64_t BudgetBytes = 0;
	float BudgetUsageRatio = 0.0f;
	RendererMemoryPressureLevel Pressure = RendererMemoryPressureLevel::Normal;
};

struct SPARKLE_RENDERER_API TextureStreamingMemoryPolicySnapshot final
{
	RendererMemoryPressureLevel OverallPressure = RendererMemoryPressureLevel::Normal;
	RendererMemoryPressureLevel TexturePressure = RendererMemoryPressureLevel::Normal;
	bool ShouldConserveTextureMemory = false;
	bool ShouldPreferMipDemotion = false;
	bool ShouldBlockMipPromotion = false;
};

struct SPARKLE_RENDERER_API SceneMemoryReport final
{
	std::uint64_t TextureBytes = 0;
	std::uint64_t MeshBytes = 0;
	std::uint64_t RayTracingBytes = 0;
	std::uint64_t UploadBytes = 0;
	std::uint64_t ConstantBufferBytes = 0;
	std::uint64_t TotalTrackedBytes = 0;
};

struct SPARKLE_RENDERER_API RendererMemoryDiagnosticsSnapshot final
{
	bool Available = false;
	std::uint64_t LastPollFrame = 0;
	std::uint32_t PollIntervalFrames = 0;
	RendererMemoryPressureLevel OverallPressure = RendererMemoryPressureLevel::Normal;
	RhiMemoryUsageSnapshot MemoryUsage;
	std::vector<RendererMemoryCategoryPressure> CategoryPressure;
	TextureStreamingMemoryPolicySnapshot TextureStreamingPolicy;
	SceneMemoryReport SceneReport;
};

SPARKLE_RENDERER_API const char* RendererMemoryPressureLevelToString(RendererMemoryPressureLevel pressure) noexcept;
