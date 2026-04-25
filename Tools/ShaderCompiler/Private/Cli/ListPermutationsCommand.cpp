#include "PCH.h"

#include "Cli/ListPermutationsCommand.h"

#include "Constants/ShaderCompilerConstants.h"
#include "Shaders/Authoring/GlobalShader.h"

#include <iostream>

int ListPermutationsCommand::Run(std::span<const std::string_view> args) const
{
	if (args.size() != 1)
	{
		std::cerr << "ShaderCompiler: list-permutations requires <shader-id>\n";
		return kExitCodeUsage;
	}

	bool foundTypedShader = false;
	for (const ShaderRegistrationDesc& shader : GlobalShaderRegistry::GetRegistrations())
	{
		if (shader.PackageName != args[0] && shader.ShaderName != args[0])
		{
			continue;
		}

		foundTypedShader = true;
		const ShaderPermutationDomainDescriptor domain =
		    shader.BuildPermutationDomainDescriptor != nullptr ? shader.BuildPermutationDomainDescriptor() : ShaderPermutationDomainDescriptor{};
		if (domain.Dimensions.empty())
		{
			std::cout << shader.PackageName << " shader=" << shader.ShaderName << " permutation=Default\n";
			continue;
		}

		std::cout << shader.PackageName << " shader=" << shader.ShaderName << " permutationDimensions=" << domain.Dimensions.size() << "\n";
	}

	if (foundTypedShader)
	{
		return kExitCodeSuccess;
	}

	std::cerr << "ShaderCompiler: unknown shader id '" << args[0] << "'\n";
	return kExitCodeUsage;
}