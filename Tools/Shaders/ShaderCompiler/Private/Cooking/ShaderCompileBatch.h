#pragma once

#include "Cooking/ShaderCompileJob.h"
#include "ToolWorkProgressEvent.h"

#include <cstddef>
#include <span>
#include <vector>

struct ShaderCookSettings;

class ShaderCompileBatch final
{
public:
	ShaderCompileBatch() = delete;

	static std::vector<ShaderCompileResult> Execute(const ShaderCookSettings& settings, std::span<const ShaderCompileJob> jobs, std::size_t totalWork, const ToolWorkProgressCallback& progress);

private:
	struct ProducerMap final
	{
		std::vector<std::size_t> ProducerJobIndices;
		std::vector<std::size_t> ProducerForJob;
	};

	static ProducerMap SelectProducers(std::span<const ShaderCompileJob> jobs);
	static std::vector<ShaderCompileResult> CompileProducers(
	    const ShaderCookSettings& settings,
	    std::span<const ShaderCompileJob> jobs,
	    const ProducerMap& producerMap,
	    std::size_t totalWork,
	    const ToolWorkProgressCallback& progress);
	static std::vector<ShaderCompileResult> FanOutResults(std::span<const ShaderCompileJob> jobs, std::span<const ShaderCompileResult> producerResults, std::span<const std::size_t> producerForJob);
	static void FinalizeResults(
	    const ShaderCookSettings& settings,
	    std::span<const ShaderCompileJob> jobs,
	    std::span<ShaderCompileResult> results,
	    std::size_t totalWork,
	    const ToolWorkProgressCallback& progress);
	static bool HasSameCompilerInput(const ShaderCompileJob& lhs, const ShaderCompileJob& rhs) noexcept;
};
