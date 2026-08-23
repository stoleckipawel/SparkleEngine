#pragma once

#include "Cooking/ShaderCookTypes.h"
#include "Cooking/ShaderCookSettings.h"

#include <string_view>
#include <unordered_set>
#include <vector>

class ShaderDependencyManifest;

class ShaderCookPlanner final
{
public:
	static std::vector<ShaderCookPackageDesc> BuildPackages(
	    const ShaderPackageCookSettings& settings,
	    const ShaderDependencyManifest& dependencyManifest);

private:
	enum class CookSelectionKind
	{
		All,
		PackageId,
		ShaderId,
		Changed,
	};

	static std::vector<ShaderCookPackageDesc> BuildTypedShaderPackages(
	    CookSelectionKind selectionKind,
	    std::string_view requestedId,
	    const std::unordered_set<ShaderTypeId>& affectedShaderTypes);
};
