#include "PCH.h"

#include "Cooking/Identity/ShaderCompileOptionsHasher.h"

#include "Compiler/ShaderCompileProfile.h"
#include "Core/Public/Hash/HashUtils.h"

std::uint64_t ShaderCompileOptionsHasher::Compute(const ShaderCompileOptions& options)
{
	std::string canonical;
	canonical.reserve(256);
	canonical += options.SourcePath;
	canonical += '|';
	canonical += ShaderCompileProfile::BuildTargetProfile(options);
	canonical += '|';
	canonical += options.EntryPoint;
	canonical += '|';
	canonical += std::to_string(static_cast<std::uint32_t>(options.Stage));
	canonical += '|';
	canonical += std::to_string(static_cast<std::uint32_t>(options.Target));
	canonical += '|';
	canonical += std::to_string(static_cast<std::uint32_t>(options.PackageKind));
	canonical += '|';
	canonical += std::to_string(static_cast<std::uint32_t>(options.PackageFeatures));
	canonical += '|';
	canonical += std::to_string(static_cast<std::uint32_t>(options.RayTracingExportKind));
	canonical += '|';
	canonical += options.EnableDebugInfo ? '1' : '0';
	canonical += options.EnableOptimizations ? '1' : '0';
	canonical += options.TreatWarningsAsErrors ? '1' : '0';
	canonical += options.StripDebugInfo ? '1' : '0';
	for (const std::string& define : options.Defines)
	{
		canonical += '|';
		canonical += define;
	}

	for (const ShaderDescriptorBindingRemap& remap : options.DescriptorBindingRemaps)
	{
		canonical += '|';
		canonical += remap.Name;
		canonical += ':';
		canonical += std::to_string(remap.Set);
		canonical += ':';
		canonical += std::to_string(remap.Binding);
	}

	const std::uint64_t hash = Hash::Fnv1a64(canonical);
	return hash != 0 ? hash : Hash::kFnv64OffsetBasis;
}
