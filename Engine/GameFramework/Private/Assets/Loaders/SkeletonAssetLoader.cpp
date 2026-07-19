#include "PCH.h"

#include "Assets/Loaders/SkeletonAssetLoader.h"

#include "Assets/Cooked/LoadedSkeletonAsset.h"
#include "Assets/Loaders/CookedAssetByteReader.h"
#include "Assets/Loaders/CookedAssetLoaderDiagnostics.h"
#include <cstdint>

namespace Assets
{
	bool SkeletonAssetLoader::Decode(
	    const std::filesystem::path& path,
	    std::span<const std::uint8_t> bytes,
	    LoadedSkeletonAsset& outSkeletonAsset,
	    std::string& outErrorMessage) const
	{
		const CookedAssetLoaderContext diagnosticsContext =
		    CookedAssetLoaderDiagnostics::BuildContext(path, "CookedSkeletonAsset", kCookedSkeletonAssetVersion);
		auto fail = [&](std::string_view recordKind, std::string_view expectedFeature, std::string_view reason) -> bool
		{
			CookedAssetLoaderDiagnostics::SetFailure(diagnosticsContext, recordKind, expectedFeature, reason, outErrorMessage);
			return false;
		};

		CookedAssetByteReader reader(bytes);
		if (!reader.Read(outSkeletonAsset.header, outErrorMessage))
		{
			return fail("header", "CookedSkeletonAssetHeader", outErrorMessage);
		}

		if (!outSkeletonAsset.header.fileHeader.Matches(kCookedSkeletonAssetMagic, kCookedSkeletonAssetVersion) ||
		    !HasValidHeader(outSkeletonAsset.header.jointStride))
		{
			return fail("header", "skeleton magic/version and joint stride", "Invalid cooked skeleton asset header");
		}

		if (!reader.ReadArray(outSkeletonAsset.header.jointCount, outSkeletonAsset.joints, outErrorMessage))
		{
			return fail("joints", "joint array matching header count", outErrorMessage);
		}

		if (reader.GetRemainingByteCount() != 0)
		{
			return fail("payload", "no trailing bytes after declared skeleton records", "Cooked skeleton asset contains unexpected trailing bytes");
		}

		outErrorMessage.clear();
		return true;
	}

	bool SkeletonAssetLoader::HasValidHeader(std::uint32_t jointStride) noexcept
	{
		return jointStride == sizeof(CookedSkeletonJointRecord);
	}
}
