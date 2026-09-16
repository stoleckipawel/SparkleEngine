#pragma once

#include <DirectXMath.h>

struct PointLight
{
	DirectX::XMFLOAT3 position = {0.0f, 0.0f, 0.0f};
	float range = 0.0f;
	DirectX::XMFLOAT3 color = {1.0f, 1.0f, 1.0f};
	// Luminous intensity in candela.
	float luminousIntensity = 1.0f;
	float radius = 0.05f;
	DirectX::XMFLOAT3 distanceAttenuationCoefficients = {0.0f, 0.0f, 1.0f};
	bool castShadow = true;
};
