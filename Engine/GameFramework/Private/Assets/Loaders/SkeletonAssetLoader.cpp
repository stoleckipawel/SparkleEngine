#include "PCH.h"

#include "Assets/Loaders/SkeletonAssetLoader.h"

#include "Assets/Cooked/LoadedSkeletonAsset.h"
#include "Assets/Loaders/CookedAssetByteReader.h"
#include "Assets/Loaders/CookedAssetLoaderDiagnostics.h"
#include "Core/Public/Strings/StringUtils.h"
#include <cstdint>

namespace Assets
{
	LoadedSkeletonAsset SkeletonAssetLoader::Decode(
	    const std::filesystem::path& path,
	    std::span<const std::uint8_t> bytes) const
	{
		const CookedAssetLoaderDiagnostics diagnostics(path, "CookedSkeletonAsset", kCookedSkeletonAssetVersion);

		CookedAssetByteReader reader(bytes);
		LoadedSkeletonAsset skeletonAsset;
		skeletonAsset.header = reader.Read<CookedSkeletonAssetHeader>();

		if (!skeletonAsset.header.fileHeader.Matches(kCookedSkeletonAssetMagic, kCookedSkeletonAssetVersion) ||
		    skeletonAsset.header.jointStride != sizeof(CookedSkeletonJointRecord))
		{
			throw diagnostics.MakeError("header", "skeleton magic/version and joint stride", "Invalid cooked skeleton asset header");
		}

		skeletonAsset.joints = reader.ReadArray<CookedSkeletonJointRecord>(skeletonAsset.header.jointCount);

		for (const CookedSkeletonJointRecord& joint : skeletonAsset.joints)
		{
			if (!Strings::IsNullTerminated(std::span(joint.name)))
			{
				throw diagnostics.MakeError("joints", "null-terminated joint names", "Cooked skeleton contains an invalid joint name");
			}
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
