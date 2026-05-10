#pragma once

#include <DirectXMath.h>

struct PerViewCameraConstantBufferData
{
	DirectX::XMFLOAT4X4 ViewMTX;
	DirectX::XMFLOAT4X4 ProjectionMTX;
	DirectX::XMFLOAT4X4 ViewProjMTX;
	DirectX::XMFLOAT4X4 InvViewMTX;
	DirectX::XMFLOAT4X4 InvProjectionMTX;

	DirectX::XMFLOAT3 Position;
	float NearZ;

	float FarZ;
	DirectX::XMFLOAT3 Direction;
};

static_assert(sizeof(PerViewCameraConstantBufferData) == 352, "Per-view camera constant buffer data must match the shader layout");