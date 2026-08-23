#pragma once

#include "Compiler/ShaderCompileRequest.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

struct IncludeClosureHash final
{
	std::uint64_t sourceHash = 0;
	std::uint64_t includeClosureHash = 0;
	std::vector<std::string> virtualDependencies;
};

class IncludeClosureHasher final
{
public:
	static IncludeClosureHash Compute(const ShaderCompileRequest& request);

private:
	using HashPair = std::pair<std::string, std::uint64_t>;

	static void VisitFile(
	    std::string_view filePath,
	    const ShaderCompileRequest& request,
	    std::unordered_set<std::string>& visitedPathKeys,
	    std::vector<HashPair>& outFileHashes);
	static std::uint64_t ComputeClosureHash(std::vector<HashPair>& fileHashes);
	static std::uint64_t FindSourceHash(std::string_view sourcePath, const std::vector<HashPair>& fileHashes);
};
