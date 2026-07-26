#pragma once

#include <DirectXMath.h>

struct RenderMeshWorldBounds final
{
	DirectX::XMFLOAT3 Min = {};
	DirectX::XMFLOAT3 Max = {};
	bool Valid = false;
};
