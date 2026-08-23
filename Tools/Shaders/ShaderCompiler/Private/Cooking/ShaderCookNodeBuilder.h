#pragma once

#include "Cooking/ShaderCookContext.h"

#include <cstddef>
#include <cstdint>
#include <string_view>

class ShaderBackendPool;
class IShaderBackend;
struct ShaderPackageCookSettings;

class ShaderCookNodeBuilder final
{
  public:
	ShaderCookNodeBuilder() = delete;

	static void BuildAndAdd(
	    const ShaderPackageCookSettings& settings,
	    std::size_t packageIndex,
	    std::size_t stageIndex,
	    std::size_t targetIndex,
	    ShaderBackendPool& backendPool,
	    ShaderCookPipelinePlan& plan);

  private:
	static ShaderCompileOptions BuildCompileOptions(
	    const ShaderPackageCookSettings& settings,
	    const ShaderCookPackageDesc& package,
	    const ShaderCookStageDesc& stage,
	    ShaderTarget target);
	static void AppendDescriptorBindingRemaps(
	    const ShaderCookPackageDesc& package,
	    ShaderCompileOptions& compileOptions);
	static std::uint64_t BuildCompileInputHash(
	    std::uint64_t sourceHash,
	    std::uint64_t includeClosureHash,
	    std::uint64_t optionsHash,
	    std::string_view backendName,
	    std::uint64_t backendVersion);
	static IShaderBackend& ResolveBackend(
	    const ShaderPackageCookSettings& settings,
	    const ShaderCookPackageDesc& package,
	    const ShaderCookStageDesc& stage,
	    ShaderTarget target,
	    const ShaderCompileOptions& compileOptions,
	    ShaderBackendPool& backendPool);
	static void AppendNode(
	    std::size_t packageIndex,
	    const ShaderCookPackageDesc& package,
	    const ShaderCookStageDesc& stage,
	    ShaderTarget target,
	    ShaderCompileOptions compileOptions,
	    const std::string& backendName,
	    const IShaderBackend& backend,
	    ShaderCookPipelinePlan& plan);
};
