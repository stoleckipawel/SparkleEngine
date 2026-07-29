#pragma once

#include "Animation/AnimationEvaluationTypes.h"

#include <DirectXMath.h>

#include <span>

namespace SkinningMatrixEvaluator
{
	bool Evaluate(
	    const ECS::SkeletonEvaluationData& skeleton,
	    std::span<const DirectX::XMFLOAT4X4> modelSpaceTransforms,
	    std::span<DirectX::XMFLOAT4X4> jointMatrices) noexcept;
}
