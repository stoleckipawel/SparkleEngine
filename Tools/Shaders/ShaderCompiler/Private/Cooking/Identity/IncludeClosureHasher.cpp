#include "PCH.h"

#include "Cooking/Identity/IncludeClosureHasher.h"

#include "Compiler/ShaderIncludeResolver.h"
#include "Compiler/ShaderSourceMountTable.h"
#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Diagnostics/Verify.h"
#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Hash/HashUtils.h"

#include <algorithm>
#include <format>
#include <regex>
#include <sstream>

static const auto g_includeClosureHasherLogger = Logging::GetOrCreateLogger("ShaderCompiler.IncludeClosureHasher");

void IncludeClosureHasher::VisitFile(
    std::string_view filePath,
    const ShaderCompileOptions& options,
    std::unordered_set<std::string>& visitedPathKeys,
    std::vector<HashPair>& outFileHashes)
{
	if (options.SourceMounts == nullptr)
	{
		throw Diagnostics::Error("Shader include hashing requires a virtual source mount table.");
	}
	const std::string pathKey = options.SourceMounts->CanonicalizeVirtualPath(filePath);
	if (!visitedPathKeys.insert(pathKey).second)
	{
		return;
	}

	std::vector<std::uint8_t> bytes;
	std::string fileError;
	const std::filesystem::path physicalPath = options.SourceMounts->ResolvePhysicalPath(pathKey);
	if (!Files::TryReadAllBytes(physicalPath, bytes, fileError))
	{
		throw Diagnostics::Error(std::format(
		    "Failed to read shader source '{}' while computing include closure.",
		    pathKey));
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
		const auto resolvedIncludePath = ShaderIncludeResolver::ResolveIncludePath(pathKey, includeSpec, options);
		if (!resolvedIncludePath)
		{
			throw Diagnostics::Error(std::format(
			    "Failed to resolve include '{}' referenced from '{}'",
			    includeSpec,
			    pathKey));
		}

		VisitFile(*resolvedIncludePath, options, visitedPathKeys, outFileHashes);
	}
}

IncludeClosureHash IncludeClosureHasher::Compute(const ShaderCompileOptions& options)
{
	std::unordered_set<std::string> visited;
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
		canonical.append(path);
		canonical += '|';
		canonical += std::to_string(hash);
		canonical += ';';
	}

	const std::uint64_t closureHash = Hash::Fnv1a64(canonical);
	return closureHash == 0 ? Hash::kFnv64OffsetBasis : closureHash;
}

std::uint64_t IncludeClosureHasher::FindSourceHash(
    std::string_view sourcePath,
    const std::vector<HashPair>& fileHashes)
{
	const std::string sourcePathKey(sourcePath);
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
