#pragma once

#include "Cooking/CookedShaderPackageOutput.h"

#include <filesystem>
#include <span>
#include <string>

class CookedRegistryWriter final
{
  public:
	static bool Write(
	    std::span<const CookedShaderPackageOutput> packages,
	    const std::filesystem::path& storagePath,
	    std::string& outErrorMessage);
};
