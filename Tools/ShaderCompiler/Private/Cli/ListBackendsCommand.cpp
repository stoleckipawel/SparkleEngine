#include "PCH.h"

#include "Cli/ListBackendsCommand.h"

#include "Backend/ShaderBackendFactory.h"
#include "Constants/ShaderCompilerConstants.h"

#include <iostream>

static std::string BuildTargetList(const ShaderBackendCapabilities& capabilities)
{
	std::string targets;
	if (capabilities.SupportsDxil)
	{
		targets = "dxil";
	}
	if (capabilities.SupportsSpirV)
	{
		if (!targets.empty())
		{
			targets += ", ";
		}
		targets += "spirv";
	}
	if (targets.empty())
	{
		targets = "none";
	}
	return targets;
}

int ListBackendsCommand::Run(std::span<const std::string_view> args) const
{
	if (!args.empty())
	{
		std::cerr << "ShaderCompiler: list-backends does not accept extra arguments\n";
		return kExitCodeUsage;
	}

	const std::vector<ShaderBackendDescriptor> backends = ListShaderBackends();
	std::cout << "ShaderCompiler: " << backends.size() << " backend(s) registered\n";
	for (const ShaderBackendDescriptor& backend : backends)
	{
		std::cout << "  name='" << backend.Name << "'"
		          << " available=" << (backend.IsAvailable ? "true" : "false")
		          << " targets='" << BuildTargetList(backend.Capabilities) << "'"
		          << " version=" << backend.Version << "\n";
	}

	return kExitCodeSuccess;
}