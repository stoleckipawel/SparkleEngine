#include "PCH.h"

#include "Cooking/Identity/IncludeClosureHasher.h"

#include "Compiler/ShaderIncludeResolver.h"
#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Diagnostics/Verify.h"
#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Hash/HashUtils.h"
#include "Core/Public/Paths/PathUtils.h"

#include <algorithm>
#include <format>
#include <regex>
#include <sstream>

static const auto g_includeClosureHasherLogger = Logging::GetOrCreateLogger("ShaderCompiler.IncludeClosureHasher");

void IncludeClosureHasher::VisitFile(
    const std::filesystem::path& filePath,
    const ShaderCompileOptions& options,
    std::unordered_set<std::wstring>& visitedPathKeys,
    std::vector<HashPair>& outFileHashes)
{
	const std::filesystem::path normalizedPath = Paths::Normalize(filePath);
	const std::wstring pathKey = ShaderIncludeResolver::MakeResolvedPathKey(normalizedPath);
	if (!visitedPathKeys.insert(pathKey).second)
	{
		return;
	}

	std::vector<std::uint8_t> bytes;
	std::string fileError;
	if (!Files::TryReadAllBytes(normalizedPath, bytes, fileError))
	{
		throw Diagnostics::Error(std::format(
		    "Failed to read shader source '{}' while computing include closure - {}",
		    normalizedPath.string(),
		    fileError));
	}

	const std::uint64_t fileHash = Hash::Fnv1a64(bytes.data(), bytes.size());
	outFileHashes.emplace_back(pathKey, fileHash == 0 ? Hash::kFnv64OffsetBasis : fileHash);

	const std::string sourceText(reinterpret_cast<const char*>(bytes.data()), bytes.size());
	static const std::regex includeRegex(R"(^\s*#\s*include\s*["<]([^">]+)[">])", std::regex::icase);
	std::smatch match;
	std::istringstream stream(sourceText);
	std::string line;
	while (std::getline(stream, line))
	{
		if (!std::regex_search(line, match, includeRegex))
		{
			continue;
		}

		const std::string includeSpec = match[1].str();
		const auto resolvedIncludePath = ShaderIncludeResolver::ResolveIncludePath(normalizedPath, includeSpec, options);
		if (!resolvedIncludePath)
		{
			throw Diagnostics::Error(std::format(
			    "Failed to resolve include '{}' referenced from '{}'",
			    includeSpec,
			    normalizedPath.string()));
		}

		VisitFile(*resolvedIncludePath, options, visitedPathKeys, outFileHashes);
	}
}

IncludeClosureHash IncludeClosureHasher::Compute(const ShaderCompileOptions& options)
{
	std::unordered_set<std::wstring> visited;
	std::vector<HashPair> fileHashes;
	VisitFile(options.SourcePath, options, visited, fileHashes);

	if (fileHashes.empty())
	{
		Diagnostics::Fatal(
		    g_includeClosureHasherLogger,
		    __FILE__,
		    __LINE__,
		    "Include closure hash computation produced no source inputs.");
	}

	return IncludeClosureHash{
	    .sourceHash = FindSourceHash(options.SourcePath, fileHashes),
	    .includeClosureHash = ComputeClosureHash(fileHashes)};
}

std::uint64_t IncludeClosureHasher::ComputeClosureHash(
    std::vector<HashPair>& fileHashes)
{
	std::sort(
	    fileHashes.begin(),
	    fileHashes.end(),
	    [](const HashPair& lhs, const HashPair& rhs)
	    {
		    return lhs.first < rhs.first;
	    });

	std::string canonical;
	canonical.reserve(fileHashes.size() * 64);
	for (const auto& [path, hash] : fileHashes)
	{
		canonical.append(
		    reinterpret_cast<const char*>(path.data()),
		    path.size() * sizeof(std::wstring::value_type));
		canonical += '|';
		canonical += std::to_string(hash);
		canonical += ';';
	}

	const std::uint64_t closureHash = Hash::Fnv1a64(canonical);
	return closureHash == 0 ? Hash::kFnv64OffsetBasis : closureHash;
}

std::uint64_t IncludeClosureHasher::FindSourceHash(
    const std::filesystem::path& sourcePath,
    const std::vector<HashPair>& fileHashes)
{
	const std::wstring sourcePathKey =
	    ShaderIncludeResolver::MakeResolvedPathKey(sourcePath);
	for (const auto& [path, hash] : fileHashes)
	{
		if (path == sourcePathKey)
		{
			return hash;
		}
	}

	Diagnostics::Fatal(
	    g_includeClosureHasherLogger,
	    __FILE__,
	    __LINE__,
	    "Include closure omitted its root shader source.");
}
