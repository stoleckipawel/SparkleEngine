#pragma once

#include "RayTracing/IndirectSpecularDebugMode.h"
#include "RayTracing/IndirectSpecularSampleMode.h"

#include <cstdint>

enum class IndirectSpecularStatusReason : std::uint32_t
{
	NotEvaluated = 0u,
	Disabled = 1u,
	Unsupported = 2u,
	MissingTlas = 3u,
	MissingHitData = 4u,
	Running = 5u,
};

struct IndirectSpecularRuntimeDiagnosticsSnapshot final
{
	IndirectSpecularStatusReason Status = IndirectSpecularStatusReason::NotEvaluated;
	const char* StatusReason = "not-evaluated";
	bool Enabled = false;
	IndirectSpecularSampleMode SampleMode = IndirectSpecularSampleMode::StochasticGGX;
	IndirectSpecularDebugMode DebugMode = IndirectSpecularDebugMode::Off;
	float MaxDistance = 0.0f;
	bool HitDataAvailable = false;
	std::uint32_t HitInstanceCount = 0;
	std::uint32_t HitMaterialCount = 0;
};

namespace IndirectSpecularRuntimeDiagnostics
{
	const char* ToString(IndirectSpecularStatusReason status) noexcept;
	IndirectSpecularRuntimeDiagnosticsSnapshot Capture() noexcept;
	void Publish(const IndirectSpecularRuntimeDiagnosticsSnapshot& snapshot) noexcept;
}
