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

static std::string BuildRayTracingFeatureList(const ShaderBackendCapabilities& capabilities)
{
	std::string features;
	if (capabilities.SupportsDxilRayTracingLibrary)
	{
		features = "dxil-rt-library";
	}
	if (capabilities.SupportsSpirVRayTracingLibrary)
	{
		if (!features.empty())
		{
			features += ", ";
		}
		features += "spirv-rt-library";
	}
	if (capabilities.SupportsDxilInlineRayQuery)
	{
		if (!features.empty())
		{
			features += ", ";
		}
		features += "dxil-inline-ray-query";
	}
	if (capabilities.SupportsSpirVInlineRayQuery)
	{
		if (!features.empty())
		{
			features += ", ";
		}
		features += "spirv-inline-ray-query";
	}
	if (features.empty())
	{
		features = "none";
	}
	return features;
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
		          << " rt='" << BuildRayTracingFeatureList(backend.Capabilities) << "'"
		          << " version=" << backend.Version << "\n";
	}

	return kExitCodeSuccess;
}