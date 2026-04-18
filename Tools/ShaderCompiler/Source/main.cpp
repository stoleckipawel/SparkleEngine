#include "Cooking/ShaderCookManifest.h"
#include "Cooking/ShaderPackageCooker.h"

#include <filesystem>
#include <iostream>
#include <string_view>

namespace
{
	bool IsInspectManifestCommand(std::string_view command) noexcept
	{
		return command == "inspect-manifest" || command == "inspect-shader-manifest";
	}

	bool IsCookCommand(std::string_view command) noexcept
	{
		return command == "cook" || command == "cook-shaders";
	}

	int RunInspectManifest()
	{
		Engine::AssetAuthoring::ShaderCookManifest manifest;
		std::string errorMessage;
		if (!manifest.LoadMerged(errorMessage))
		{
			std::cerr << "ShaderCompiler: failed to validate shader cook manifest - " << errorMessage << "\n";
			return 5;
		}

		std::cout << "ShaderCompiler: shader cook manifest ready. Engine manifest='"
		          << Engine::AssetAuthoring::ShaderCookManifest::GetEngineManifestPath().string() << "'";
		const std::filesystem::path projectManifestPath = Engine::AssetAuthoring::ShaderCookManifest::GetProjectManifestPath();
		if (!projectManifestPath.empty())
		{
			std::cout << ", project manifest='" << projectManifestPath.string() << "'";
		}
		std::cout << "\n";

		std::cout << "ShaderCompiler: cooked shader output root='"
		          << Engine::AssetAuthoring::ShaderCookManifest::GetCookedShaderPackageRoot().string() << "'"
		          << ", registry='" << Engine::AssetAuthoring::ShaderCookManifest::GetCookedShaderRegistryPath().string() << "'\n";

		for (const Engine::AssetAuthoring::ShaderCookPackageDesc& package : manifest.GetPackages())
		{
			const std::uint64_t packageKey =
			    Engine::AssetAuthoring::ShaderCookManifest::BuildShaderPackageKey(package.packageId, package.variantId);
			std::cout << "  Package '" << package.packageId << "' variant='" << package.variantId << "' bindingLayout='"
			          << package.bindingLayoutId << "' key=" << std::hex << packageKey << std::dec
			          << " output='"
			          << Engine::AssetAuthoring::ShaderCookManifest::BuildCookedShaderPackagePath(packageKey).string()
			          << "'\n";

			for (const Engine::AssetAuthoring::ShaderCookStageDesc& stage : package.stages)
			{
				std::cout << "    - " << GetShaderStagePrefix(stage.stage) << ": " << stage.sourcePath.string()
				          << " | entry=" << stage.entryPoint << "\n";
			}
		}

		return 0;
	}

	int RunCookShaders()
	{
		Engine::AssetAuthoring::ShaderPackageCooker cooker;
		const Engine::AssetAuthoring::ShaderPackageCookResult cookResult = cooker.CookAll();
		if (!cookResult.Succeeded())
		{
			std::cerr << "ShaderCompiler: failed to cook shader packages - " << cookResult.errorMessage << "\n";
			return 6;
		}

		std::cout << "ShaderCompiler: cooked " << cookResult.packages.size() << " shader package(s) under '"
		          << Engine::AssetAuthoring::ShaderCookManifest::GetCookedShaderPackageRoot().string() << "'"
		          << " and registry '" << cookResult.registryPath.string() << "'\n";

		for (const Engine::AssetAuthoring::CookedShaderPackageOutput& package : cookResult.packages)
		{
			std::cout << "  Package '" << package.packageId << "' variant='" << package.variantId << "' bindingLayout='"
			          << package.bindingLayoutId << "' key=" << std::hex << package.packageKey << std::dec
			          << " output='" << package.outputPath.string() << "'\n";
		}

		return 0;
	}
}  // namespace

int main(int argc, char** argv)
{
	if (argc == 2)
	{
		const std::string_view command(argv[1]);
		if (IsInspectManifestCommand(command))
		{
			return RunInspectManifest();
		}

		if (IsCookCommand(command))
		{
			return RunCookShaders();
		}
	}

	std::cerr << "Usage:\n"
	          << "  ShaderCompiler inspect-manifest\n"
	          << "  ShaderCompiler cook\n"
	          << "\n"
	          << "Compatibility:\n"
	          << "  ShaderCompiler inspect-shader-manifest\n"
	          << "  ShaderCompiler cook-shaders\n";
	return 1;
}