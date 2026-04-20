#include "PCH.h"

#include "Cli/CookShadersCommand.h"

#include "Constants/ShaderCompilerConstants.h"
#include "Cooking/ShaderPackageCooker.h"
#include "RHI/Public/Shaders/CookedShaderPackageUtils.h"

#include <iostream>

int CookShadersCommand::Run() const
{
	ShaderPackageCooker cooker;
	const ShaderPackageCookResult cookResult = cooker.CookAll();
	if (!cookResult.Succeeded())
	{
		std::cerr << "ShaderCompiler: failed to cook shader packages - " << cookResult.errorMessage << "\n";
		return kExitCodeCookFailure;
	}

	std::cout << "ShaderCompiler: cooked " << cookResult.packages.size() << " shader package(s) under '"
	          << ::GetCookedShaderPackageRootPath().string() << "'"
	          << " and registry '" << cookResult.registryPath.string() << "'\n";

	for (const CookedShaderPackageOutput& package : cookResult.packages)
	{
		std::cout << "  Package '" << package.packageId << "' variant='" << package.variantId << "' bindingLayout='"
		          << package.bindingLayoutId << "' key=" << std::hex << package.packageKey << std::dec
		          << " output='" << package.outputPath.string() << "'\n";
	}

	return kExitCodeSuccess;
}

