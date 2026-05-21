#include "PCH.h"

#include "Cooking/Cache/IncludeClosureHasher.h"

#include "Compiler/ShaderIncludeResolver.h"
#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Hash/HashUtils.h"
#include "Core/Public/Paths/PathUtils.h"

#include <algorithm>
#include <format>
#include <regex>
#include <sstream>

bool IncludeClosureHasher::ResolveValidationInclude(
	const std::filesystem::path& includerPath,
	std::string_view includePath,
	const ShaderCompileOptions& options,
	std::string& outErrorMessage)
{
	if (includePath.empty())
	{
		return true;
	}

	if (ShaderIncludeResolver::ResolveIncludePath(includerPath, includePath, options))
	{
		return true;
	}

	outErrorMessage = std::format(
		"Failed to resolve include '{}' referenced from '{}'",
		includePath,
		Paths::Normalize(includerPath).string());
	return false;
}

bool IncludeClosureHasher::VisitFile(
	const std::filesystem::path& filePath,
	const ShaderCompileOptions& options,
	std::unordered_set<std::wstring>& visitedPathKeys,
	std::vector<HashPair>& outFileHashes,
	std::string& outErrorMessage)
{
	const std::filesystem::path normalizedPath = Paths::Normalize(filePath);
	const std::wstring pathKey = ShaderIncludeResolver::MakeResolvedPathKey(normalizedPath);
	if (!visitedPathKeys.insert(pathKey).second)
	{
		return true;
	}

	std::vector<std::uint8_t> bytes;
	if (!Files::TryReadAllBytes(normalizedPath, bytes, outErrorMessage))
	{
		outErrorMessage = std::format(
			"Failed to read shader source '{}' while computing include closure - {}",
			normalizedPath.string(),
			outErrorMessage);
		return false;
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
			outErrorMessage = std::format(
				"Failed to resolve include '{}' referenced from '{}'",
				includeSpec,
				normalizedPath.string());
			return false;
		}

		if (!VisitFile(*resolvedIncludePath, options, visitedPathKeys, outFileHashes, outErrorMessage))
		{
			return false;
		}
	}

	return true;
}

IncludeClosureHashResult IncludeClosureHasher::Compute(const ShaderCompileOptions& options)
{
	IncludeClosureHashResult result;
	std::unordered_set<std::wstring> visited;
	std::vector<HashPair> fileHashes;

	if (!VisitFile(options.SourcePath, options, visited, fileHashes, result.errorMessage))
	{
		return result;
	}

	if (fileHashes.empty())
	{
		result.errorMessage = "Include closure hash computation produced no source inputs";
		return result;
	}

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

	result.includeClosureHash = Hash::Fnv1a64(canonical);
	if (result.includeClosureHash == 0)
	{
		result.includeClosureHash = Hash::kFnv64OffsetBasis;
	}

	const std::wstring sourcePathKey = ShaderIncludeResolver::MakeResolvedPathKey(options.SourcePath);
	for (const auto& [path, hash] : fileHashes)
	{
		if (path == sourcePathKey)
		{
			result.sourceHash = hash;
			break;
		}
	}

	return result;
}
