#include "PCH.h"

#include "Cooking/Cache/ShaderCompileOptionsHasher.h"

#include "Core/Public/Hash/HashUtils.h"
#include "Core/Public/Paths/PathUtils.h"

std::uint64_t ShaderCompileOptionsHasher::Compute(const ShaderCompileOptions& options)
{
	auto appendPathKey = [](std::string& target, const std::filesystem::path& path)
	{
		const std::wstring key = Engine::Paths::MakePathKey(path);
		target.append(
			reinterpret_cast<const char*>(key.data()),
			key.size() * sizeof(std::wstring::value_type));
	};

	std::string canonical;
	canonical.reserve(256);
	canonical += options.BuildTargetProfile();
	canonical += '|';
	canonical += options.EntryPoint;
	canonical += '|';
	canonical += std::to_string(static_cast<std::uint32_t>(options.Stage));
	canonical += '|';
	canonical += std::to_string(static_cast<std::uint32_t>(options.Target));
	canonical += '|';
	canonical += options.EnableDebugInfo ? '1' : '0';
	canonical += options.EnableOptimizations ? '1' : '0';
	canonical += options.TreatWarningsAsErrors ? '1' : '0';
	canonical += options.StripReflection ? '1' : '0';
	canonical += options.StripDebugInfo ? '1' : '0';
	canonical += '|';
	appendPathKey(canonical, options.IncludeDir);

	for (const std::filesystem::path& includeDir : options.AdditionalIncludeDirs)
	{
		canonical += '|';
		appendPathKey(canonical, includeDir);
	}

	for (const std::string& define : options.Defines)
	{
		canonical += '|';
		canonical += define;
	}

	const std::uint64_t hash = Hash::Fnv1a64(canonical);
	return hash != 0 ? hash : Hash::kFnv64OffsetBasis;
}
