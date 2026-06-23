#pragma once

#include "RayTracing/Effects/IndirectDiffuse/IndirectDiffuseDebugMode.h"

struct IndirectDiffuseSettings final
{
	bool Enabled = false;
	IndirectDiffuseDebugMode DebugMode = IndirectDiffuseDebugMode::Off;
	float NormalBias = 0.01f;
	float MaxDistance = 100000.0f;
	float Intensity = 1.0f;
};

IndirectDiffuseSettings BuildIndirectDiffuseSettingsFromCVars() noexcept;
