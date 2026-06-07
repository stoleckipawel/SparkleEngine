#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"

struct SPARKLE_ENGINE_API PointLightDesc
{
	float range = 0.0f;
	float sourceRadius = 0.05f;
	bool castShadow = true;
};
