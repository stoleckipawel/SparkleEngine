#pragma once

#include "Manifest/ShaderCookManifestTypes.h"

#include <filesystem>
#include <functional>
#include <map>
#include <string>
#include <string_view>

using ShaderCookPackageMap = std::map<std::string, ShaderCookPackageDesc, std::less<>>;

class ShaderCookManifestParser final
{
  public:
	static bool ParseInto(
	    const std::filesystem::path& manifestPath,
	    ShaderCookPackageMap& inOutPackages,
	    std::string& outErrorMessage);

	static std::string MakePackageLookupKey(std::string_view packageId);

  private:
	static bool ParseStageValue(std::string_view value, ShaderCookStageDesc& outStage, std::string& outErrorMessage);
};
