#include "PCH.h"

#include "Cli/InspectShaderCommand.h"

#include "Constants/ShaderCompilerConstants.h"
#include "Contracts/ShaderContractCatalogBuilder.h"
#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Formatting/HexFormat.h"
#include "Inspection/CookedPackageInspection.h"
#include "RHI/Public/Shaders/CookedShaderPackageIdentity.h"

#include <iostream>
#include <string>

int InspectShaderCommand::Run(std::span<const std::string_view> args) const
{
	if (args.empty())
	{
		std::cerr << "ShaderCompiler: inspect-shader requires <shader-id>\n";
		return kExitCodeUsage;
	}

	ShaderContractCatalog catalog;
	try
	{
		catalog = ShaderContractCatalogBuilder::Build(ShaderContractSelectionKind::RegisteredId, args[0]);
	}
	catch (const Diagnostics::Error&)
	{
		std::cerr << "ShaderCompiler: unknown shader id '" << args[0] << "'\n";
		return kExitCodeUsage;
	}

	for (const ShaderContractPackage& package : catalog.packages)
	{
		std::cout << "Typed shader package '" << package.packageId << "'\n";
		std::cout << "  packageKey=" << Formatting::FormatPrefixedHexUInt64(BuildShaderPackageKey(package.packageId)) << "\n";
		std::cout << "  bindingLayout='" << package.bindingLayoutId << "'"
		          << " layoutHash=" << Formatting::FormatPrefixedHexUInt64(BuildPassParameterLayoutHash(package.bindingLayout))
		          << " parameters=" << package.bindingLayout.GetParameters().size() << "\n";
		std::cout << "  kind=" << CookedPackageInspection::GetPackageKindName(package.packageKind)
		          << " features='" << CookedPackageInspection::FormatPackageFeatures(package.packageFeatures) << "'\n";
		for (const ShaderContractStage& stage : package.stages)
		{
			std::cout << "  shader='" << stage.shaderName << "' " << GetShaderStagePrefix(stage.stage)
			          << ": " << stage.sourcePath.generic_string() << " entry=" << stage.entryPoint
			          << " parameters=" << stage.parameterStruct.Fields.size() << "\n";
		}
	}

	return kExitCodeSuccess;
}
