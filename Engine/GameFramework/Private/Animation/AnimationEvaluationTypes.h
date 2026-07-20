#pragma once

#include "GameFramework/Public/Scene/Skeletons/SkeletonResource.h"

#include <DirectXMath.h>

#include <span>

namespace ECS
{
	struct AnimationJointTransform final
	{
		DirectX::XMFLOAT3 Translation{0.0f, 0.0f, 0.0f};
		DirectX::XMFLOAT4 Rotation{0.0f, 0.0f, 0.0f, 1.0f};
		DirectX::XMFLOAT3 Scale{1.0f, 1.0f, 1.0f};
	};

	struct SkeletonEvaluationData final
	{
		const SkeletonResource* Resource = nullptr;
		std::span<const AnimationJointTransform> BindLocalTransforms;

		bool IsValid() const noexcept
		{
			return Resource != nullptr && BindLocalTransforms.size() == Resource->joints.size();
		}
	};
}
