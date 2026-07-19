#include "PCH.h"

#include "Assets/Loaders/AnimationAssetLoader.h"

#include "Assets/Cooked/LoadedAnimationAsset.h"
#include "Assets/Loaders/CookedAssetByteReader.h"
#include "Assets/Loaders/CookedAssetLoaderDiagnostics.h"
#include <cstdint>

namespace Assets
{
	bool AnimationAssetLoader::Decode(
	    const std::filesystem::path& path,
	    std::span<const std::uint8_t> bytes,
	    LoadedAnimationAsset& outAnimationAsset,
	    std::string& outErrorMessage) const
	{
		const CookedAssetLoaderContext diagnosticsContext =
		    CookedAssetLoaderDiagnostics::BuildContext(path, "CookedAnimationAsset", kCookedAnimationAssetVersion);
		auto fail = [&](std::string_view recordKind, std::string_view expectedFeature, std::string_view reason) -> bool
		{
			CookedAssetLoaderDiagnostics::SetFailure(diagnosticsContext, recordKind, expectedFeature, reason, outErrorMessage);
			return false;
		};

		CookedAssetByteReader reader(bytes);
		if (!reader.Read(outAnimationAsset.header, outErrorMessage))
		{
			return fail("header", "CookedAnimationAssetHeader", outErrorMessage);
		}

		if (!outAnimationAsset.header.fileHeader.Matches(kCookedAnimationAssetMagic, kCookedAnimationAssetVersion) ||
		    !HasValidHeader(outAnimationAsset.header.channelStride, outAnimationAsset.header.keyframeStride))
		{
			return fail("header", "animation magic/version plus channel and keyframe strides", "Invalid cooked animation asset header");
		}

		if (!reader.ReadArray(outAnimationAsset.header.channelCount, outAnimationAsset.channels, outErrorMessage) ||
		    !reader.ReadArray(outAnimationAsset.header.keyframeCount, outAnimationAsset.keyframes, outErrorMessage))
		{
			return fail("payload", "channel and keyframe arrays matching header counts", outErrorMessage);
		}

		if (reader.GetRemainingByteCount() != 0)
		{
			return fail("payload", "no trailing bytes after declared animation records", "Cooked animation asset contains unexpected trailing bytes");
		}

		outErrorMessage.clear();
		return true;
	}

	bool AnimationAssetLoader::HasValidHeader(std::uint32_t channelStride, std::uint32_t keyframeStride) noexcept
	{
		return channelStride == sizeof(CookedAnimationChannelRecord) && keyframeStride == sizeof(CookedAnimationKeyframeRecord);
	}
}
