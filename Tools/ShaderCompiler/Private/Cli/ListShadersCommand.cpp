#include "PCH.h"

#include "Cli/ListShadersCommand.h"

#include "Constants/ShaderCompilerConstants.h"
#include "Shaders/Authoring/GlobalShader.h"

#include <iostream>

int ListShadersCommand::Run(std::span<const std::string_view> args) const
{
	const bool validateOnly = args.size() == 1 && args[0] == "--validate";
	if (!args.empty() && !validateOnly)
	{
		std::cerr << "ShaderCompiler: list-shaders accepts only optional --validate\n";
		return kExitCodeUsage;
	}

	const std::span<const ShaderRegistrationDesc> typedShaders =
	    GlobalShaderRegistry::GetRegistrations();

	if (validateOnly)
	{
		std::cout << "ShaderCompiler: " << typedShaders.size() << " typed shader registration(s) valid\n";
		return kExitCodeSuccess;
	}

	if (!typedShaders.empty())
	{
		std::cout << "Typed shader registrations:\n";
		for (const ShaderRegistrationDesc& shader : typedShaders)
		{
			const ShaderParameterStructDescriptor parameters =
			    shader.BuildParameterStructDescriptor != nullptr ? shader.BuildParameterStructDescriptor() : ShaderParameterStructDescriptor{};
			const ShaderPermutationDomainDescriptor permutations =
			    shader.BuildPermutationDomainDescriptor != nullptr ? shader.BuildPermutationDomainDescriptor() : ShaderPermutationDomainDescriptor{};

			std::cout << shader.ShaderName << " package=" << GetShaderRegistrationPackageId(shader)
			          << " layout=" << GetShaderRegistrationBindingLayoutId(shader)
			          << " stage=" << GetShaderStagePrefix(shader.Stage)
			          << " source=" << shader.SourcePath
			          << " entry=" << shader.EntryPoint
			          << " parameters=" << parameters.Fields.size()
			          << " permutationDimensions=" << permutations.Dimensions.size() << "\n";
		}
	}
	return kExitCodeSuccess;
}