#pragma once

#include "Cooking/ShaderCookSettings.h"
#include "Cooking/ShaderCookTypes.h"
#include "ShaderContractCatalog.h"

#include <vector>

class ShaderDependencyManifest;

class ShaderCookPlanner final
{
public:
	static std::vector<ShaderCookDesc> BuildShaders(
	    const ShaderCookSettings& settings,
	    const ShaderDependencyManifest& dependencyManifest,
	    const ShaderContractCatalog& catalog);
};
