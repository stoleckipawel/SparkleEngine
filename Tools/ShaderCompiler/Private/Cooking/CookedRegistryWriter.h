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
	    std::filesystem::path& outRegistryPath,
	    std::string& outErrorMessage);
};
