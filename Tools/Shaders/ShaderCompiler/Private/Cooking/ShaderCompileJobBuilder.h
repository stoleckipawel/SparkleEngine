#pragma once

#include "Cooking/ShaderCookContext.h"

#include <cstddef>
#include <cstdint>
#include <string_view>

class ShaderBackendPool;
struct ShaderCookSettings;

class ShaderCompileJobBuilder final
{
public:
	ShaderCompileJobBuilder() = delete;

	static void BuildAndAdd(
	    const ShaderCookSettings& settings,
	    std::size_t shaderIndex,
	    ShaderTarget target,
	    ShaderBackendPool& backendPool,
	    ShaderCookPipelinePlan& plan);

private:
	static ShaderCompileRequest BuildRequest(const ShaderCookSettings& settings, const ShaderCookDesc& shader, ShaderTarget target);
	static void AppendDescriptorBindingRemaps(const ShaderCookDesc& shader, ShaderCompileRequest& request);
	static ShaderCompileFeatureFlags BuildRequiredFeatures(ShaderFeatureFlags features) noexcept;
	static const ShaderSourceMountTable& GetSourceMounts();
	static ShaderCompileInputHash BuildInputHash(
	    std::uint64_t sourceContentHash,
	    std::uint64_t dependencyClosureHash,
	    std::uint64_t requestHash,
	    std::string_view backendName,
	    std::uint64_t backendVersion);
};
