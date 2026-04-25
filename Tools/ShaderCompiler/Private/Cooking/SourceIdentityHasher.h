#pragma once

#include "Cooking/CookedStageBuild.h"
#include "Cooking/ShaderCookTypes.h"

#include <cstdint>
#include <span>

class SourceIdentityHasher final
{
  public:
	static std::uint64_t Compute(
	    const ShaderCookPackageDesc& package,
	    std::span<const CookedStageBuild> compiledStages);
};
