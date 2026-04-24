#pragma once

#include "ShaderCompileOptions.h"

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

struct IncludeClosureHashResult final
{
	std::uint64_t sourceHash = 0;
	std::uint64_t includeClosureHash = 0;
	std::string errorMessage;

	bool Succeeded() const noexcept { return errorMessage.empty(); }
};

class IncludeClosureHasher final
{
  public:
	static IncludeClosureHashResult Compute(const ShaderCompileOptions& options);

  private:
	using HashPair = std::pair<std::wstring, std::uint64_t>;

	static std::optional<std::filesystem::path> ResolveIncludePath(
		const std::filesystem::path& includerPath,
		std::string_view includePath,
		const ShaderCompileOptions& options);

	static bool VisitFile(
		const std::filesystem::path& filePath,
		const ShaderCompileOptions& options,
		std::unordered_set<std::wstring>& visitedPathKeys,
		std::vector<HashPair>& outFileHashes,
		std::string& outErrorMessage);
};
