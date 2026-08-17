#include "PCH.h"

#include "Assets/Loaders/SkeletonAssetLoader.h"

#include "Assets/Cooked/LoadedSkeletonAsset.h"
#include "Assets/Loaders/CookedAssetByteReader.h"
#include "Assets/Loaders/CookedAssetLoaderDiagnostics.h"
#include "Core/Public/Strings/StringUtils.h"
#include "GameFramework/Public/Scene/Animations/SkeletonTransformContract.h"

#include <cstdint>
#include <limits>
#include <span>
#include <unordered_set>
#include <vector>

namespace Assets
{
	LoadedSkeletonAsset SkeletonAssetLoader::Decode(const std::filesystem::path& path, std::span<const std::uint8_t> bytes) const
	{
		const CookedAssetLoaderDiagnostics diagnostics(path, "CookedSkeletonAsset", kCookedSkeletonAssetVersion);

		CookedAssetByteReader reader(bytes);
		LoadedSkeletonAsset skeletonAsset;
		skeletonAsset.header = reader.Read<CookedSkeletonAssetHeader>();

		if (!skeletonAsset.header.fileHeader.Matches(kCookedSkeletonAssetMagic, kCookedSkeletonAssetVersion)
		    || skeletonAsset.header.coordinateContractVersion != WorldCoordinates::kCoordinateContractVersion
		    || skeletonAsset.header.jointStride != sizeof(CookedSkeletonJointRecord))
		{
			throw diagnostics.MakeError(
			    "header",
			    "skeleton magic/version, current coordinate contract, and joint stride",
			    "Invalid cooked skeleton asset header; recook the spatial asset");
		}

		skeletonAsset.joints = reader.ReadArray<CookedSkeletonJointRecord>(skeletonAsset.header.jointCount);

		std::unordered_set<std::uint32_t> sourceNodeIndices;
		for (std::size_t jointIndex = 0; jointIndex < skeletonAsset.joints.size(); ++jointIndex)
		{
			const CookedSkeletonJointRecord& joint = skeletonAsset.joints[jointIndex];
			const DirectX::XMFLOAT4X4* parentBindModel = joint.parentJointIndex < skeletonAsset.joints.size()
			    ? &skeletonAsset.joints[joint.parentJointIndex].bindModelTransform
			    : nullptr;
			if (!Strings::IsNullTerminated(std::span(joint.name)) || joint.sourceNodeIndex == (std::numeric_limits<std::uint32_t>::max)()
			    || !sourceNodeIndices.insert(joint.sourceNodeIndex).second
			    || (joint.parentJointIndex != kInvalidCookedSkeletonJointIndex
			        && (joint.parentJointIndex >= skeletonAsset.joints.size() || joint.parentJointIndex == jointIndex))
			    || !SkeletonTransformContract::IsFinite(joint.inverseBindMatrix)
			    || !SkeletonTransformContract::IsFinite(joint.bindLocalTransform)
			    || !SkeletonTransformContract::IsFinite(joint.parentSpaceTransform)
			    || !SkeletonTransformContract::IsFinite(joint.bindModelTransform)
			    || !SkeletonTransformContract::IsInvertible(joint.inverseBindMatrix)
			    || !SkeletonTransformContract::IsInvertible(joint.bindLocalTransform)
			    || !SkeletonTransformContract::IsTrsDecomposable(joint.bindLocalTransform)
			    || !SkeletonTransformContract::SatisfiesBindInvariant(
			        joint.bindLocalTransform,
			        joint.parentSpaceTransform,
			        parentBindModel,
			        joint.bindModelTransform))
			{
				throw diagnostics.MakeError(
				    "joints",
				    "unique nodes, valid parents, finite invertible TRS, and the canonical bind invariant",
				    "Cooked skeleton violates the current transform contract; recook the spatial asset");
			}
		}
		std::vector<std::uint32_t> evaluationOrder;
		if (skeletonAsset.joints.empty() || !SkeletonTransformContract::BuildEvaluationOrder(skeletonAsset.joints, evaluationOrder))
		{
			throw diagnostics.MakeError(
			    "joints",
			    "one or more joints in an acyclic hierarchy",
			    "Cooked skeleton has an empty or cyclic hierarchy; recook the spatial asset");
		}

		if (reader.GetRemainingByteCount() != 0)
		{
			throw diagnostics.MakeError(
			    "payload",
			    "no trailing bytes after declared skeleton records",
			    "Cooked skeleton asset contains unexpected trailing bytes");
		}

		return skeletonAsset;
	}
}
