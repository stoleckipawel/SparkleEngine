#include "PCH.h"

#include "GameFramework/Public/Assets/Cooked/CookedAssetCommon.h"

bool Assets::CookedAssetHeader::Matches(
    std::uint32_t expectedMagic,
    std::uint32_t expectedVersion) const noexcept
{
	return magic == expectedMagic &&
	       version == expectedVersion;
}
