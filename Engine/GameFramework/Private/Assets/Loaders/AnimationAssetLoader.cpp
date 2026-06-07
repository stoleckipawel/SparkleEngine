#include "PCH.h"

#include "Assets/Loaders/AnimationAssetLoader.h"

#include "Assets/Cooked/LoadedAnimationAsset.h"
#include "Assets/Loaders/CookedAssetByteReader.h"
#include "Core/Public/Files/FileUtils.h"

#include <cstdint>
#include <vector>

namespace Assets
{
	bool AnimationAssetLoader::Load(const std::filesystem::path& path, LoadedAnimationAsset& outAnimationAsset, std::string& outErrorMessage) const
	{
		std::vector<std::uint8_t> fileBytes;
		if (!Files::TryReadAllBytes(path, fileBytes, outErrorMessage))
		{
			return false;
		}

		CookedAssetByteReader reader(fileBytes);
		if (!reader.Read(outAnimationAsset.header, outErrorMessage))
		{
			return false;
		}

		if (!outAnimationAsset.header.fileHeader.Matches(kCookedAnimationAssetMagic, kCookedAnimationAssetVersion) ||
		    !HasValidHeader(outAnimationAsset.header.channelStride, outAnimationAsset.header.keyframeStride))
		{
			outErrorMessage = "Invalid cooked animation asset header";
			return false;
		}

		if (!reader.ReadArray(outAnimationAsset.header.channelCount, outAnimationAsset.channels, outErrorMessage) ||
		    !reader.ReadArray(outAnimationAsset.header.keyframeCount, outAnimationAsset.keyframes, outErrorMessage))
		{
			return false;
		}

		if (reader.GetRemainingByteCount() != 0)
		{
			outErrorMessage = "Cooked animation asset contains unexpected trailing bytes";
			return false;
		}

		outErrorMessage.clear();
		return true;
	}

	bool AnimationAssetLoader::HasValidHeader(std::uint32_t channelStride, std::uint32_t keyframeStride) noexcept
	{
		return channelStride == sizeof(CookedAnimationChannelRecord) && keyframeStride == sizeof(CookedAnimationKeyframeRecord);
	}
}
