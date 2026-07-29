#pragma once

#include "ShaderCompileOptions.h"

#include <cstdint>
#include <optional>
#include <unordered_set>
#include <utility>
#include <vector>

struct IncludeClosureHash final
{
	std::uint64_t sourceHash = 0;
	std::uint64_t includeClosureHash = 0;
};

class IncludeClosureHasher final
{
  public:
	static IncludeClosureHash Compute(const ShaderCompileOptions& options);

  private:
	using HashPair = std::pair<std::wstring, std::uint64_t>;

	static void VisitFile(
	    const std::filesystem::path& filePath,
	    const ShaderCompileOptions& options,
	    std::unordered_set<std::wstring>& visitedPathKeys,
	    std::vector<HashPair>& outFileHashes);
	static std::uint64_t ComputeClosureHash(
	    std::vector<HashPair>& fileHashes);
	static std::uint64_t FindSourceHash(
	    const std::filesystem::path& sourcePath,
	    const std::vector<HashPair>& fileHashes);
};
