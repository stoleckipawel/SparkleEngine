#include "PCH.h"

#include "Cli/ListBackendsCommand.h"

#include "Backend/ShaderBackendFactory.h"
#include "Constants/ShaderCompilerConstants.h"

#include <iostream>

static std::string BuildStringList(std::span<const std::string_view> values)
{
	std::string result;
	for (const std::string_view value : values)
	{
		if (!result.empty())
		{
			result += ", ";
		}
		result += value;
	}
	if (result.empty())
	{
		result = "none";
	}
	return result;
}

static std::string BuildTargetList(std::span<const ShaderTarget> targets)
{
	std::string result;
	for (const ShaderTarget target : targets)
	{
		if (!result.empty())
		{
			result += ", ";
		}
		result += GetShaderTargetName(target);
	}
	if (result.empty())
	{
		result = "none";
	}
	return result;
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
		          << " required=" << (backend.IsRequired ? "true" : "false")
		          << " available=" << (backend.IsAvailable ? "true" : "false")
		          << " sourceExtensions='" << BuildStringList(backend.SourceExtensions) << "'"
		          << " codegenTargets='" << BuildTargetList(backend.CodegenTargets) << "'"
		          << " binaryFormats='" << BuildStringList(backend.BinaryFormats) << "'"
		          << " dependencies='" << BuildStringList(backend.DependencyLocations) << "'"
		          << " rt='" << BuildRayTracingFeatureList(backend.Capabilities) << "'"
		          << " version=" << backend.Version;
		if (!backend.UnavailableReason.empty())
		{
			std::cout << " unavailableReason='" << backend.UnavailableReason << "'";
		}
		std::cout << "\n";
	}

	const std::span<const ShaderBinaryFormatDescriptor> binaryFormats = ListShaderBinaryFormats();
	std::cout << "ShaderCompiler: " << binaryFormats.size() << " binary format(s) registered\n";
	for (const ShaderBinaryFormatDescriptor& binaryFormat : binaryFormats)
	{
		std::cout << "  name='" << binaryFormat.Name << "'"
		          << " available=" << (binaryFormat.IsAvailable ? "true" : "false") << "\n";
	}

	const std::span<const ShaderCodegenTargetDescriptor> codegenTargets = ListShaderCodegenTargets();
	std::cout << "ShaderCompiler: " << codegenTargets.size() << " codegen target(s) registered\n";
	for (const ShaderCodegenTargetDescriptor& codegenTarget : codegenTargets)
	{
		std::cout << "  name='" << GetShaderTargetName(codegenTarget.Target) << "'"
		          << " binaryFormat='" << codegenTarget.BinaryFormat << "'"
		          << " available=" << (codegenTarget.IsAvailable ? "true" : "false") << "\n";
	}

	return kExitCodeSuccess;
}