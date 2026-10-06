#include "PCH.h"

#include "Types/ImportedScene.h"

#include <limits>

std::pair<ImportedSkeletonIndex, std::uint32_t> ImportedScene::FindSkeletonJointForNode(std::uint32_t sourceNodeIndex) const noexcept
{
	for (std::size_t skeletonIndex = 0; skeletonIndex < skeletons.size(); ++skeletonIndex)
	{
		const ImportedSkeleton& skeleton = skeletons[skeletonIndex];
		for (std::size_t jointIndex = 0; jointIndex < skeleton.joints.size(); ++jointIndex)
		{
			if (skeleton.joints[jointIndex].sourceNodeIndex == sourceNodeIndex)
			{
				return {static_cast<std::uint32_t>(skeletonIndex), static_cast<std::uint32_t>(jointIndex)};
			}
		}
	}

	return {(std::numeric_limits<std::uint32_t>::max)(), (std::numeric_limits<std::uint32_t>::max)()};
}
