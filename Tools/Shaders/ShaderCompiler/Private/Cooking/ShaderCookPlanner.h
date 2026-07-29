#pragma once

#include "Cooking/ShaderCookTypes.h"
#include "Cooking/ShaderCookSettings.h"
#include "ShaderCompileOptions.h"

#include <string_view>
#include <vector>

class ShaderCookPlanner final
{
  public:
	static ShaderCompileOptions BuildCompileOptions(const ShaderCookStageDesc& stage);
	static std::vector<ShaderCookPackageDesc> BuildPackages(const ShaderPackageCookSettings& settings);
  private:
	enum class CookSelectionKind
	{
		All,
		PackageId,
		ShaderId,
	};

	static std::vector<ShaderCookPackageDesc> BuildTypedShaderPackages(
	    CookSelectionKind selectionKind,
	    std::string_view requestedId);
};
