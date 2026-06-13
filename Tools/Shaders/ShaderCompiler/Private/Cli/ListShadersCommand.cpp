#include "PCH.h"

#include "Cli/ListShadersCommand.h"

#include "Constants/ShaderCompilerConstants.h"
#include "Contracts/ShaderContractCatalogBuilder.h"
#include "Contracts/ShaderContractValidator.h"

#include <iostream>
#include <string>

int ListShadersCommand::Run(std::span<const std::string_view> args) const
{
	const bool validateOnly = args.size() == 1 && args[0] == "--validate";
	if (!args.empty() && !validateOnly)
	{
		std::cerr << "ShaderCompiler: list-shaders accepts only optional --validate\n";
		return kExitCodeUsage;
	}

	std::string errorMessage;
	const ShaderContractCatalog catalog =
	    ShaderContractCatalogBuilder::Build(ShaderContractSelectionKind::All, {}, errorMessage);
	if (!errorMessage.empty())
	{
		std::cerr << "ShaderCompiler: failed to build shader contract catalog - " << errorMessage << "\n";
		return kExitCodeCookFailure;
	}

	const std::vector<ShaderContractVerificationFailure> failures = ShaderContractValidator::Validate(catalog);
	if (validateOnly)
	{
		for (const ShaderContractVerificationFailure& failure : failures)
		{
			std::cerr << "ShaderCompiler: shader contract invalid "
			          << ShaderContractValidator::FormatFailure(failure) << "\n";
		}
		if (!failures.empty())
		{
			std::cerr << "ShaderCompiler: " << failures.size() << " shader contract validation error(s)\n";
			return kExitCodeCookFailure;
		}
		std::cout << "ShaderCompiler: " << catalog.stages.size() << " typed shader registration(s) valid"
		          << " packages=" << catalog.packages.size() << "\n";
		return kExitCodeSuccess;
	}

	if (!catalog.stages.empty())
	{
		std::cout << "Typed shader registrations:\n";
		for (const ShaderContractStage& stage : catalog.stages)
		{
			std::cout << stage.shaderName << " package=" << stage.packageId
			          << " layout=" << stage.bindingLayoutId
			          << " stage=" << GetShaderStagePrefix(stage.stage)
			          << " source=" << stage.sourcePath.generic_string()
			          << " entry=" << stage.entryPoint
			          << " parameters=" << stage.parameterStruct.Fields.size() << "\n";
		}
	}
	return kExitCodeSuccess;
}
