#include "PCH.h"

#include "Cli/ListTargetsCommand.h"

#include "RHI/Public/Shaders/ShaderTarget.h"
#include "Constants/ShaderCompilerConstants.h"

#include <iostream>

int ListShaderTargets(std::span<const std::string_view> args)
{
	if (!args.empty())
	{
		std::cerr << "ShaderCompiler: list-targets does not accept extra arguments\n";
		return kExitCodeUsage;
	}

	for (std::uint16_t candidate = static_cast<std::uint16_t>(ShaderTarget::DxilSm60); candidate <= static_cast<std::uint16_t>(ShaderTarget::SpirV16); ++candidate)
	{
		std::cout << GetShaderTargetName(static_cast<ShaderTarget>(candidate)) << '\n';
	}
	return kExitCodeSuccess;
}
