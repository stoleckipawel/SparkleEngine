#include "PCH.h"

#include "Cli/InspectShaderCommand.h"

#include "Constants/ShaderCompilerConstants.h"
#include "Contracts/ShaderContractCatalogBuilder.h"

#include <iostream>
#include <string>

int InspectShaderCommand::Run(std::span<const std::string_view> args) const
{
	if (args.empty())
	{
		std::cerr << "ShaderCompiler: inspect-shader requires <shader-id>\n";
		return kExitCodeUsage;
	}

	std::string errorMessage;
	ShaderContractCatalog catalog =
	    ShaderContractCatalogBuilder::Build(ShaderContractSelectionKind::ShaderId, args[0], errorMessage);
	if (!errorMessage.empty())
	{
		catalog = ShaderContractCatalogBuilder::Build(ShaderContractSelectionKind::PackageId, args[0], errorMessage);
	}
	if (!errorMessage.empty())
	{
		std::cerr << "ShaderCompiler: unknown shader id '" << args[0] << "'\n";
		return kExitCodeUsage;
	}

	for (const ShaderContractPackage& package : catalog.packages)
	{
		std::cout << "Typed shader package '" << package.packageId << "'\n";
		std::cout << "  bindingLayout='" << package.bindingLayoutId << "'\n";
		for (const ShaderContractStage& stage : package.stages)
		{
			std::cout << "  shader='" << stage.shaderName << "' " << GetShaderStagePrefix(stage.stage)
			          << ": " << stage.sourcePath.generic_string() << " entry=" << stage.entryPoint
			          << " parameters=" << stage.parameterStruct.Fields.size() << "\n";
		}
	}

	return kExitCodeSuccess;
}
