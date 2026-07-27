#pragma once

#include "Cooking/ShaderCookContext.h"

#include <cstddef>
#include <string>

class ShaderBackendPool;
class IShaderBackend;
struct ShaderPackageCookSettings;

class ShaderCookNodeBuilder final
{
  public:
	ShaderCookNodeBuilder() = delete;

	static bool BuildAndAdd(
	    const ShaderPackageCookSettings& settings,
	    std::size_t packageIndex,
	    std::size_t stageIndex,
	    std::size_t targetIndex,
	    ShaderBackendPool& backendPool,
	    ShaderCookPipelinePlan& plan,
	    std::string& outErrorMessage);

  private:
	static ShaderCompileOptions BuildCompileOptions(
	    const ShaderPackageCookSettings& settings,
	    const ShaderCookPackageDesc& package,
	    const ShaderCookStageDesc& stage,
	    ShaderTarget target);
	static void AppendDescriptorBindingRemaps(
	    const ShaderCookPackageDesc& package,
	    ShaderCompileOptions& compileOptions);
	static bool ResolveBackend(
	    const ShaderPackageCookSettings& settings,
	    const ShaderCookPackageDesc& package,
	    const ShaderCookStageDesc& stage,
	    ShaderTarget target,
	    const ShaderCompileOptions& compileOptions,
	    ShaderBackendPool& backendPool,
	    IShaderBackend*& outBackend,
	    std::string& outBackendName,
	    std::string& outErrorMessage);
	static bool AppendNode(
	    std::size_t packageIndex,
	    const ShaderCookPackageDesc& package,
	    const ShaderCookStageDesc& stage,
	    ShaderTarget target,
	    ShaderCompileOptions compileOptions,
	    const std::string& backendName,
	    const IShaderBackend& backend,
	    ShaderCookPipelinePlan& plan,
	    std::string& outErrorMessage);
};
