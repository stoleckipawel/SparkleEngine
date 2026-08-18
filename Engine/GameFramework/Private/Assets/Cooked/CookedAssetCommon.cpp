#include "PCH.h"

#include "GameFramework/Public/Assets/Cooked/CookedAssetCommon.h"

bool Assets::CookedAssetHeader::HasMagic(std::uint32_t expectedMagic) const noexcept
{
	return magic == expectedMagic;
}
