#include "PCH.h"

#include "Animation/SkinningMatrixEvaluator.h"

namespace SkinningMatrixEvaluator
{
	bool Evaluate(
	    const ECS::SkeletonEvaluationData& skeleton,
	    std::span<const DirectX::XMFLOAT4X4> modelSpaceTransforms,
	    std::span<DirectX::XMFLOAT4X4> skinningMatrices) noexcept
	{
		if (!skeleton.IsValid() || modelSpaceTransforms.size() != skeleton.Resource->joints.size() ||
		    skinningMatrices.size() != skeleton.Resource->joints.size())
			return false;
		for (std::size_t jointIndex = 0; jointIndex < skeleton.Resource->joints.size(); ++jointIndex)
		{
			const DirectX::XMMATRIX skinning =
			    DirectX::XMLoadFloat4x4(&skeleton.Resource->joints[jointIndex].inverseBindMatrix) *
			    DirectX::XMLoadFloat4x4(&modelSpaceTransforms[jointIndex]);
			DirectX::XMStoreFloat4x4(&skinningMatrices[jointIndex], skinning);
		}
		return true;
	}
}
