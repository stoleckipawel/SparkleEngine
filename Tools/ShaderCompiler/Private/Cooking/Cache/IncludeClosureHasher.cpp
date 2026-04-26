#include "PCH.h"

#include "Cooking/Cache/IncludeClosureHasher.h"

#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Hash/HashUtils.h"
#include "Core/Public/Paths/PathUtils.h"

#include <algorithm>
#include <format>
#include <regex>
#include <sstream>

std::optional<std::filesystem::path> IncludeClosureHasher::ResolveIncludePath(
	const std::filesystem::path& includerPath,
	std::string_view includePath,
	const ShaderCompileOptions& options)
{
	const std::filesystem::path includeRelativePath(includePath);

	std::error_code ec;
	const std::filesystem::path localCandidate = Paths::Normalize(includerPath.parent_path() / includeRelativePath);
	if (std::filesystem::exists(localCandidate, ec) && !ec)
	{
		return localCandidate;
	}

	auto checkRoot = [&](const std::filesystem::path& root) -> std::optional<std::filesystem::path>
	{
		if (root.empty())
		{
			return std::nullopt;
		}

		ec.clear();
		const std::filesystem::path candidate = Paths::Normalize(root / includeRelativePath);
		if (std::filesystem::exists(candidate, ec) && !ec)
		{
			return candidate;
		}
		return std::nullopt;
	};

	if (const auto fromPrimary = checkRoot(options.IncludeDir))
	{
		return fromPrimary;
	}

	for (const std::filesystem::path& includeRoot : options.AdditionalIncludeDirs)
	{
		if (const auto fromAdditional = checkRoot(includeRoot))
		{
			return fromAdditional;
		}
	}

	return std::nullopt;
}

bool IncludeClosureHasher::VisitFile(
	const std::filesystem::path& filePath,
	const ShaderCompileOptions& options,
	std::unordered_set<std::wstring>& visitedPathKeys,
	std::vector<HashPair>& outFileHashes,
	std::string& outErrorMessage)
{
	const std::filesystem::path normalizedPath = Paths::Normalize(filePath);
	const std::wstring pathKey = Paths::MakePathKey(normalizedPath);
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
		const auto resolvedIncludePath = ResolveIncludePath(normalizedPath, includeSpec, options);
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

	const std::wstring sourcePathKey = Paths::MakePathKey(Paths::Normalize(options.SourcePath));
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
