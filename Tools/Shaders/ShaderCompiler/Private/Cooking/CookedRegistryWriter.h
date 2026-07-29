#pragma once

#include "Cooking/CookedShaderPackageOutput.h"

#include <filesystem>
#include <span>

class CookedRegistryWriter final
{
  public:
	static void Write(
	    std::span<const CookedShaderPackageOutput> packages,
	    const std::filesystem::path& storagePath);
};
