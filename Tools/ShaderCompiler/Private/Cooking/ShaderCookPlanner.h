#pragma once

#include "Cooking/ShaderCookTypes.h"
#include "Cooking/ShaderPackageCooker.h"
#include "ShaderCompileOptions.h"
#include "Shaders/Authoring/ShaderParameterStruct.h"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

class ShaderCookPlanner final
{
  public:
	static ShaderCompileOptions BuildCompileOptions(const ShaderCookStageDesc& stage);
	static std::vector<ShaderCookPackageDesc> BuildPackages(
	    const ShaderPackageCookSettings& settings,
	    std::string& outErrorMessage);
	static std::optional<ShaderParameterStructDescriptor> FindParameterStructDescriptor(
	    const ShaderCompileOptions& options);

  private:
	static std::vector<ShaderCookPackageDesc> BuildSingleShaderPackage(const ShaderPackageCookSettings& settings);
	static std::vector<ShaderCookPackageDesc> BuildTypedShaderPackages(
	    std::string_view requestedPackageId,
	    std::string& outErrorMessage);
};