#include "PCH.h"

#include "Cli/ListShadersCommand.h"

#include "Constants/ShaderCompilerConstants.h"
#include "Contracts/ShaderContractCatalogBuilder.h"
#include "Contracts/ShaderContractValidator.h"
#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Formatting/HexFormat.h"

#include <iostream>

int ListShadersCommand::Run(std::span<const std::string_view> args) const
{
	const bool validateOnly = args.size() == 1 && args[0] == "--validate";
	if (!args.empty() && !validateOnly)
	{
		std::cerr << "ShaderCompiler: list-shaders accepts only optional --validate\n";
		return kExitCodeUsage;
	}

	ShaderContractCatalog catalog;
	try
	{
		catalog = ShaderContractCatalogBuilder::Build(ShaderContractSelectionKind::All, {});
	}
	catch (const Diagnostics::Error& error)
	{
		std::cerr << "ShaderCompiler: failed to build shader catalog - " << error.what() << "\n";
		return kExitCodeCookFailure;
	}
	const std::vector<ShaderContractVerificationFailure> failures = ShaderContractValidator::Validate(catalog);
	for (const ShaderContractVerificationFailure& failure : failures)
	{
		std::cerr << "ShaderCompiler: shader contract invalid " << ShaderContractValidator::FormatFailure(failure) << "\n";
	}
	if (!failures.empty())
	{
		return kExitCodeCookFailure;
	}
	if (validateOnly)
	{
		std::cout << "ShaderCompiler: " << catalog.size() << " typed shader registration(s) valid\n";
		return kExitCodeSuccess;
	}
	for (const ShaderContract& shader : catalog)
	{
		std::cout << shader.shaderName << " type=" << Formatting::FormatPrefixedHexUInt64(shader.shaderTypeId)
		          << " stage=" << GetShaderStagePrefix(shader.stage) << " source=" << shader.sourcePath
		          << " entry=" << shader.entryPoint << " parameters=" << shader.parameterStruct.Fields.size() << "\n";
	}
	return kExitCodeSuccess;
}
