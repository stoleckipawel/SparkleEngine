#pragma once

#include <DirectXMath.h>

struct PerViewCameraConstantBufferData
{
	DirectX::XMFLOAT3 Position;
	float NearZ;

	float FarZ;
	DirectX::XMFLOAT3 Direction;
};

static_assert(sizeof(PerViewCameraConstantBufferData) == 32, "Per-view camera constant buffer data must be 32 bytes");