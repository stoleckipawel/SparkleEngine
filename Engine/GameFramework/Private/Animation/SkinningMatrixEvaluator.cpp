#include "PCH.h"

#include "Animation/SkinningMatrixEvaluator.h"

namespace SkinningMatrixEvaluator
{
	bool Evaluate(
	    const ECS::SkeletonEvaluationData& skeleton,
	    std::span<const DirectX::XMFLOAT4X4> modelSpaceTransforms,
	    std::span<DirectX::XMFLOAT4X4> jointMatrices) noexcept
	{
		if (!skeleton.IsValid() || modelSpaceTransforms.size() != skeleton.Resource->joints.size()
		    || jointMatrices.size() != skeleton.Resource->joints.size())
			return false;
		for (std::size_t jointIndex = 0; jointIndex < skeleton.Resource->joints.size(); ++jointIndex)
		{
			const DirectX::XMMATRIX skinningMatrix = DirectX::XMLoadFloat4x4(&skeleton.Resource->joints[jointIndex].inverseBindMatrix)
			    * DirectX::XMLoadFloat4x4(&modelSpaceTransforms[jointIndex]);
			DirectX::XMStoreFloat4x4(&jointMatrices[jointIndex], skinningMatrix);
		}
		return true;
	}
}
