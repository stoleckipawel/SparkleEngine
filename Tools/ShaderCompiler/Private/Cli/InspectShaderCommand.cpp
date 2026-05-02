#include "PCH.h"

#include "Cli/InspectShaderCommand.h"

#include "Constants/ShaderCompilerConstants.h"
#include "Shaders/Authoring/GlobalShader.h"

#include <iostream>

static void PrintPermutationDetails(const ShaderPermutationDomainDescriptor& permutations)
{
	if (permutations.Dimensions.empty())
	{
		std::cout << "\n";
		return;
	}

	const std::vector<ShaderPermutationVector> vectors = EnumerateShaderPermutationVectors(permutations);
	std::cout << " permutations=" << vectors.size() << "\n";
	for (const ShaderPermutationDimensionDescriptor& dimension : permutations.Dimensions)
	{
		std::cout << "    permutationDimension name='" << dimension.Name
		          << "' define='" << (dimension.DefineName.empty() ? "<none>" : dimension.DefineName)
		          << "' values='";
		for (std::size_t valueIndex = 0; valueIndex < dimension.Values.size(); ++valueIndex)
		{
			if (valueIndex > 0)
			{
				std::cout << ", ";
			}
			const ShaderPermutationValueDescriptor& value = dimension.Values[valueIndex];
			std::cout << valueIndex << ':' << value.Name << '=' << value.DefineValue;
		}
		std::cout << "'\n";
	}
}

int InspectShaderCommand::Run(std::span<const std::string_view> args) const
{
	if (args.empty())
	{
		std::cerr << "ShaderCompiler: inspect-shader requires <shader-id>\n";
		return kExitCodeUsage;
	}

	bool foundTypedShader = false;
	for (const ShaderRegistrationDesc& shader : GlobalShaderRegistry::GetRegistrations())
	{
		const std::string packageName = GetShaderRegistrationPackageId(shader);
		if (packageName != args[0] && shader.ShaderName != args[0])
		{
			continue;
		}

		if (!foundTypedShader)
		{
			std::cout << "Typed shader package '" << packageName << "'\n";
			std::cout << "  bindingLayout='" << GetShaderRegistrationBindingLayoutId(shader) << "'\n";
		}
		foundTypedShader = true;

		const ShaderParameterStructDescriptor parameters =
		    shader.BuildParameterStructDescriptor != nullptr ? shader.BuildParameterStructDescriptor() : ShaderParameterStructDescriptor{};
		const ShaderPermutationDomainDescriptor permutations =
		    shader.BuildPermutationDomainDescriptor != nullptr ? shader.BuildPermutationDomainDescriptor() : ShaderPermutationDomainDescriptor{};

		std::cout << "  shader='" << shader.ShaderName << "' " << GetShaderStagePrefix(shader.Stage)
		          << ": " << shader.SourcePath << " entry=" << shader.EntryPoint
		          << " parameters=" << parameters.Fields.size()
		          << " permutationDimensions=" << permutations.Dimensions.size();
		PrintPermutationDetails(permutations);
	}

	if (foundTypedShader)
	{
		return kExitCodeSuccess;
	}

	std::cerr << "ShaderCompiler: unknown shader id '" << args[0] << "'\n";
	return kExitCodeUsage;
}