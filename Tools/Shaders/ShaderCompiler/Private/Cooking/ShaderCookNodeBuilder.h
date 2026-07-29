#pragma once

#include "Cooking/ShaderCookContext.h"

#include <cstddef>

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
