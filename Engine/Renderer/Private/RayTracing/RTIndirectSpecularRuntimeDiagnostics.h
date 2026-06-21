#pragma once

#include "RayTracing/RTIndirectSpecularDebugMode.h"
#include "RayTracing/RTIndirectSpecularSampleMode.h"

#include <cstdint>

enum class RTIndirectSpecularStatusReason : std::uint32_t
{
	NotEvaluated = 0u,
	Disabled = 1u,
	Unsupported = 2u,
	MissingTlas = 3u,
	MissingHitData = 4u,
	Running = 5u,
};

struct RTIndirectSpecularRuntimeDiagnosticsSnapshot final
{
	RTIndirectSpecularStatusReason Status = RTIndirectSpecularStatusReason::NotEvaluated;
	const char* StatusReason = "not-evaluated";
	bool Enabled = false;
	RTIndirectSpecularSampleMode SampleMode = RTIndirectSpecularSampleMode::StochasticGGX;
	RTIndirectSpecularDebugMode DebugMode = RTIndirectSpecularDebugMode::Off;
	float MaxDistance = 0.0f;
	bool HitDataAvailable = false;
	std::uint32_t HitInstanceCount = 0;
	std::uint32_t HitMaterialCount = 0;
};

namespace RTIndirectSpecularRuntimeDiagnostics
{
	const char* ToString(RTIndirectSpecularStatusReason status) noexcept;
	RTIndirectSpecularRuntimeDiagnosticsSnapshot Capture() noexcept;
	void Publish(const RTIndirectSpecularRuntimeDiagnosticsSnapshot& snapshot) noexcept;
}
