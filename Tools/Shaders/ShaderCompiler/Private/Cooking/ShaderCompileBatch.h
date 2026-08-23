#pragma once

#include "Cooking/ShaderCompileJob.h"

#include <cstddef>
#include <span>
#include <vector>

struct ShaderPackageCookSettings;

class ShaderCompileBatch final
{
public:
	ShaderCompileBatch() = delete;

	static std::vector<ShaderCompileResult> Execute(const ShaderPackageCookSettings& settings, std::span<const ShaderCompileJob> jobs);

private:
	struct ProducerMap final
	{
		std::vector<std::size_t> ProducerJobIndices;
		std::vector<std::size_t> ProducerForJob;
	};

	static ProducerMap SelectProducers(std::span<const ShaderCompileJob> jobs);
	static std::vector<ShaderCompileResult> CompileProducers(
	    const ShaderPackageCookSettings& settings,
	    std::span<const ShaderCompileJob> jobs,
	    std::span<const std::size_t> producerJobIndices);
	static std::vector<ShaderCompileResult> FanOutResults(
	    std::span<const ShaderCompileJob> jobs,
	    std::span<const ShaderCompileResult> producerResults,
	    std::span<const std::size_t> producerForJob);
	static void FinalizeResults(
	    const ShaderPackageCookSettings& settings,
	    std::span<const ShaderCompileJob> jobs,
	    std::span<ShaderCompileResult> results);
	static bool HasSameCompilerInput(const ShaderCompileJob& lhs, const ShaderCompileJob& rhs) noexcept;
};
