#include "PCH.h"

#include "Cli/InspectShaderCommand.h"

#include "Constants/ShaderCompilerConstants.h"
#include "Shaders/Authoring/GlobalShader.h"

#include <iostream>

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
		if (shader.PackageName != args[0] && shader.ShaderName != args[0])
		{
			continue;
		}

		if (!foundTypedShader)
		{
			std::cout << "Typed shader package '" << shader.PackageName << "'\n";
			std::cout << "  bindingLayout='" << shader.BindingLayoutId << "'\n";
		}
		foundTypedShader = true;

		const ShaderParameterStructDescriptor parameters =
		    shader.BuildParameterStructDescriptor != nullptr ? shader.BuildParameterStructDescriptor() : ShaderParameterStructDescriptor{};
		const ShaderPermutationDomainDescriptor permutations =
		    shader.BuildPermutationDomainDescriptor != nullptr ? shader.BuildPermutationDomainDescriptor() : ShaderPermutationDomainDescriptor{};

		std::cout << "  shader='" << shader.ShaderName << "' " << GetShaderStagePrefix(shader.Stage)
		          << ": " << shader.SourcePath << " entry=" << shader.EntryPoint
		          << " parameters=" << parameters.Fields.size()
		          << " permutationDimensions=" << permutations.Dimensions.size() << "\n";
	}

	if (foundTypedShader)
	{
		return kExitCodeSuccess;
	}

	std::cerr << "ShaderCompiler: unknown shader id '" << args[0] << "'\n";
	return kExitCodeUsage;
}