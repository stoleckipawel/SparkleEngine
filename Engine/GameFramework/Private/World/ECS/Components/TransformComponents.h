#pragma once

#include "Core/Public/Math/MathUtils.h"

#include <DirectXMath.h>

namespace ECS
{
	struct LocalTransform final
	{
		DirectX::XMFLOAT3 Translation{0.0f, 0.0f, 0.0f};
		DirectX::XMFLOAT4 Rotation{0.0f, 0.0f, 0.0f, 1.0f};
		DirectX::XMFLOAT3 Scale{1.0f, 1.0f, 1.0f};
	};

	struct WorldTransform final
	{
		DirectX::XMFLOAT4X4 Matrix = MathUtils::IdentityFloat4x4();
	};
}
