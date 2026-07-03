#pragma once

#include "RayTracing/Effects/IndirectSpecular/IndirectSpecularDebugMode.h"
#include "RayTracing/Effects/IndirectSpecular/IndirectSpecularSampleMode.h"
#include "SceneData/MaterialBindingMode.h"

#include <cstdint>

struct IndirectSpecularSettings final
{
	bool Enabled = false;
	IndirectSpecularSampleMode SampleMode = IndirectSpecularSampleMode::StochasticGGX;
	IndirectSpecularDebugMode DebugMode = IndirectSpecularDebugMode::Off;
	MaterialBindingMode MaterialMode = MaterialBindingMode::RaytracingOnly;
	float NormalBias = 0.0f;
	float MaxDistance = 0.0f;
	std::uint32_t BounceCount = 1u;
};

IndirectSpecularSettings BuildIndirectSpecularSettingsFromCVars() noexcept;
