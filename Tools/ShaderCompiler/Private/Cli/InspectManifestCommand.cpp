#include "PCH.h"

#include "Cli/InspectManifestCommand.h"

#include "Constants/ShaderCompilerConstants.h"
#include "Manifest/ShaderCookManifest.h"
#include "RHI/Public/Shaders/CookedShaderPackageUtils.h"

#include <iostream>

int InspectManifestCommand::Run() const
{
	ShaderCookManifest manifest;
	std::string errorMessage;
	if (!manifest.LoadMerged(errorMessage))
	{
		std::cerr << "ShaderCompiler: failed to validate shader cook manifest - " << errorMessage << "\n";
		return kExitCodeManifestFailure;
	}

	std::cout << "ShaderCompiler: shader cook manifest ready. Engine manifest='"
	          << ShaderCookManifest::GetEngineManifestPath().string() << "'";
	const std::filesystem::path projectManifestPath = ShaderCookManifest::GetProjectManifestPath();
	if (!projectManifestPath.empty())
	{
		std::cout << ", project manifest='" << projectManifestPath.string() << "'";
	}
	std::cout << "\n";

	std::cout << "ShaderCompiler: cooked shader output root='"
	          << ::GetCookedShaderPackageRootPath().string() << "'"
	          << ", registry='" << ::GetCookedShaderRegistryPath().string() << "'\n";

	for (const ShaderCookPackageDesc& package : manifest.GetPackages())
	{
		const std::uint64_t packageKey = ::BuildShaderPackageKey(package.packageId, package.variantId);
		std::cout << "  Package '" << package.packageId << "' variant='" << package.variantId << "' bindingLayout='"
		          << package.bindingLayoutId << "' key=" << std::hex << packageKey << std::dec
		          << " output='" << ::BuildCookedShaderPackagePath(packageKey).string() << "'\n";

		for (const ShaderCookStageDesc& stage : package.stages)
		{
			std::cout << "    - " << GetShaderStagePrefix(stage.stage) << ": " << stage.sourcePath.string()
			          << " | entry=" << stage.entryPoint << "\n";
		}
	}

	return kExitCodeSuccess;
}

