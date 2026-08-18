#pragma once

#include "GameFramework/Public/Scene/Camera/CameraDesc.h"

#include <DirectXMath.h>

struct RenderViewCameraData final
{
	DirectX::XMFLOAT3 Position{0.0f, 0.0f, 0.0f};
	DirectX::XMFLOAT3 Direction{0.0f, 0.0f, 1.0f};
	float FovYDegrees = 60.0f;
	float AspectRatio = 1.0f;
	float NearZ = 0.1f;
	float FarZ = 1000.0f;
	float OrthographicHeightMeters = 10.0f;
	CameraProjectionKind ProjectionKind = CameraProjectionKind::Perspective;
};
