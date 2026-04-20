#pragma once

#include "Manifest/ShaderCookManifestTypes.h"

#include <filesystem>
#include <string>

class ShaderCookManifestValidator final
{
  public:
	static bool Validate(
	    const ShaderCookPackageDesc& package,
	    const std::filesystem::path& manifestPath,
	    std::string& outErrorMessage);
};
