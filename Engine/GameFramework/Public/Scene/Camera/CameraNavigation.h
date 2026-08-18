#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Scene/Camera/CameraInputIntent.h"
#include "GameFramework/Public/Scene/Camera/CameraNavigationSettings.h"

#include <DirectXMath.h>

struct SPARKLE_ENGINE_API CameraNavigationState final
{
	DirectX::XMFLOAT3 Position{0.0f, 0.0f, 0.0f};
	float YawRadians = 0.0f;
	float PitchRadians = 0.0f;
};

class SPARKLE_ENGINE_API CameraNavigation final
{
public:
	static bool Apply(
	    const CameraInputIntent& intent,
	    const CameraNavigationSettings& settings,
	    float deltaSeconds,
	    CameraNavigationState& state) noexcept;

	static DirectX::XMFLOAT3 BuildDirection(const CameraNavigationState& state) noexcept;
	static float ApplySpeedSteps(float speedMetersPerSecond, float stepCount, float minimumSpeed, float maximumSpeed) noexcept;
};
