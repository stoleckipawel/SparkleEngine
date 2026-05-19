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
	enum class CookSelectionKind
	{
		All,
		PackageId,
		ShaderId,
	};

	static std::vector<ShaderCookPackageDesc> BuildTypedShaderPackages(
	    CookSelectionKind selectionKind,
	    std::string_view requestedId,
	    std::string& outErrorMessage);
};