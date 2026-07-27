#include "PCH.h"

#include "Shaders/CookedShaderPackage.h"

CookedShaderPackageFeatureFlags operator|(
    CookedShaderPackageFeatureFlags lhs,
    CookedShaderPackageFeatureFlags rhs) noexcept
{
	return static_cast<CookedShaderPackageFeatureFlags>(
	    static_cast<std::uint32_t>(lhs) |
	    static_cast<std::uint32_t>(rhs));
}

CookedShaderPackageFeatureFlags& operator|=(
    CookedShaderPackageFeatureFlags& lhs,
    CookedShaderPackageFeatureFlags rhs) noexcept
{
	lhs = lhs | rhs;
	return lhs;
}

bool HasCookedShaderPackageFeature(
    CookedShaderPackageFeatureFlags value,
    CookedShaderPackageFeatureFlags flag) noexcept
{
	return (
	           static_cast<std::uint32_t>(value) &
	           static_cast<std::uint32_t>(flag)) != 0;
}

bool CookedShaderPackageHeader::Matches(
    std::uint32_t expectedMagic,
    std::uint32_t expectedVersion) const noexcept
{
	return Magic == expectedMagic &&
	       Version == expectedVersion;
}
