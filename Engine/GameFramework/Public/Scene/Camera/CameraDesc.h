#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"

#include <DirectXMath.h>

enum class CameraProjectionKind
{
	Perspective = 0,
	Orthographic,
	Unknown
};

struct SPARKLE_ENGINE_API CameraDesc
{
	DirectX::XMFLOAT3 position{0.0f, 0.0f, -4.0f};
	float yawRadians = 0.0f;
	float pitchRadians = 0.0f;
	float fovYDegrees = 60.0f;
	float nearZ = 0.1f;
	float farZ = 1000.0f;
	float moveSpeed = 0.15f;
	CameraProjectionKind projectionKind = CameraProjectionKind::Perspective;
};
