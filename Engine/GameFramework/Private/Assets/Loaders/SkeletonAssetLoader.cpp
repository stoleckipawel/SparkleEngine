#include "PCH.h"

#include "Assets/Loaders/SkeletonAssetLoader.h"

#include "Assets/Cooked/LoadedSkeletonAsset.h"
#include "Assets/Loaders/CookedAssetByteReader.h"
#include "Core/Public/Files/FileUtils.h"

#include <cstdint>
#include <vector>

namespace Assets
{
	bool SkeletonAssetLoader::Load(const std::filesystem::path& path, LoadedSkeletonAsset& outSkeletonAsset, std::string& outErrorMessage) const
	{
		std::vector<std::uint8_t> fileBytes;
		if (!Files::TryReadAllBytes(path, fileBytes, outErrorMessage))
		{
			return false;
		}

		CookedAssetByteReader reader(fileBytes);
		if (!reader.Read(outSkeletonAsset.header, outErrorMessage))
		{
			return false;
		}

		if (!outSkeletonAsset.header.fileHeader.Matches(kCookedSkeletonAssetMagic, kCookedSkeletonAssetVersion) ||
		    !HasValidHeader(outSkeletonAsset.header.jointStride))
		{
			outErrorMessage = "Invalid cooked skeleton asset header";
			return false;
		}

		if (!reader.ReadArray(outSkeletonAsset.header.jointCount, outSkeletonAsset.joints, outErrorMessage))
		{
			return false;
		}

		if (reader.GetRemainingByteCount() != 0)
		{
			outErrorMessage = "Cooked skeleton asset contains unexpected trailing bytes";
			return false;
		}

		outErrorMessage.clear();
		return true;
	}

	bool SkeletonAssetLoader::HasValidHeader(std::uint32_t jointStride) noexcept
	{
		return jointStride == sizeof(CookedSkeletonJointRecord);
	}
}
