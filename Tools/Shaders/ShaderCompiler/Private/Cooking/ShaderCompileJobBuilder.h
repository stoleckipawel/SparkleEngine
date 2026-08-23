#pragma once

#include "Cooking/ShaderCookContext.h"

#include <cstddef>
#include <cstdint>
#include <string_view>

class ShaderBackendPool;
struct ShaderPackageCookSettings;

class ShaderCompileJobBuilder final
{
public:
	ShaderCompileJobBuilder() = delete;

	static void BuildAndAdd(
	    const ShaderPackageCookSettings& settings,
	    std::size_t packageIndex,
	    std::size_t stageIndex,
	    ShaderTarget target,
	    ShaderBackendPool& backendPool,
	    ShaderCookPipelinePlan& plan);

private:
	static ShaderCompileRequest BuildRequest(
	    const ShaderPackageCookSettings& settings,
	    const ShaderCookPackageDesc& package,
	    const ShaderCookStageDesc& stage,
	    ShaderTarget target);
	static void AppendDescriptorBindingRemaps(const ShaderCookPackageDesc& package, ShaderCompileRequest& request);
	static ShaderCompileFeatureFlags BuildRequiredFeatures(CookedShaderPackageFeatureFlags packageFeatures) noexcept;
	static const ShaderSourceMountTable& GetSourceMounts();
	static ShaderCompileInputHash BuildInputHash(
	    std::uint64_t sourceContentHash,
	    std::uint64_t dependencyClosureHash,
	    std::uint64_t requestHash,
	    std::string_view backendName,
	    std::uint64_t backendVersion);
};
