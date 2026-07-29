#include "PCH.h"

#include "Assets/Loaders/AnimationAssetLoader.h"

#include "Assets/Cooked/LoadedAnimationAsset.h"
#include "Assets/Loaders/CookedAssetByteReader.h"
#include "Assets/Loaders/CookedAssetLoaderDiagnostics.h"
#include "Core/Public/Strings/StringUtils.h"
#include <cstdint>

namespace Assets
{
	LoadedAnimationAsset AnimationAssetLoader::Decode(
	    const std::filesystem::path& path,
	    std::span<const std::uint8_t> bytes) const
	{
		const CookedAssetLoaderDiagnostics diagnostics(path, "CookedAnimationAsset", kCookedAnimationAssetVersion);

		CookedAssetByteReader reader(bytes);
		LoadedAnimationAsset animationAsset;
		animationAsset.header = reader.Read<CookedAnimationAssetHeader>();

		if (!animationAsset.header.fileHeader.Matches(kCookedAnimationAssetMagic, kCookedAnimationAssetVersion) ||
		    animationAsset.header.channelStride != sizeof(CookedAnimationChannelRecord) ||
		    animationAsset.header.keyframeStride != sizeof(CookedAnimationKeyframeRecord) ||
		    !Strings::IsNullTerminated(std::span(animationAsset.header.name)))
		{
			throw diagnostics.MakeError(
			    "header",
			    "animation magic/version plus channel and keyframe strides",
			    "Invalid cooked animation asset header");
		}

		animationAsset.channels = reader.ReadArray<CookedAnimationChannelRecord>(animationAsset.header.channelCount);
		animationAsset.keyframes = reader.ReadArray<CookedAnimationKeyframeRecord>(animationAsset.header.keyframeCount);

		if (reader.GetRemainingByteCount() != 0)
		{
			throw diagnostics.MakeError(
			    "payload",
			    "no trailing bytes after declared animation records",
			    "Cooked animation asset contains unexpected trailing bytes");
		}

		return animationAsset;
	}
}
