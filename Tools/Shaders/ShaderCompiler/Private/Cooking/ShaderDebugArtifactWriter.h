#pragma once

#include "Cooking/CookedStageBuild.h"
#include "Cooking/ShaderCookTypes.h"
#include "ShaderDebugArtifactSet.h"
#include "ShaderCompileOptions.h"

#include <filesystem>
#include <string>

class ShaderDebugArtifactWriter final
{
  public:
	static void Write(
	    const std::filesystem::path& rootDirectory,
	    const ShaderCookPackageDesc& package,
	    const ShaderCookStageDesc& stage,
	    const ShaderCompileOptions& options,
	    const CookedStageBuild& compiledStage,
	    const ShaderDebugArtifactSet& debugArtifacts);

  private:
	static void WriteText(const std::filesystem::path& path, std::string_view contents);
	static void WriteCompileInputs(
	    const std::filesystem::path& bundleDirectory,
	    const ShaderCookPackageDesc& package,
	    const ShaderCookStageDesc& stage,
	    const ShaderCompileOptions& options,
	    const CookedStageBuild& compiledStage);
	static void WriteCompilerOutputs(
	    const std::filesystem::path& bundleDirectory,
	    const ShaderDebugArtifactSet& debugArtifacts,
	    const CookedStageBuild& compiledStage);
	static std::string BuildBundleDirectoryName(
	    const ShaderCookPackageDesc& package,
	    const ShaderCookStageDesc& stage,
	    const ShaderCompileOptions& options,
	    const CookedStageBuild& compiledStage);
	static std::string BuildCompileRequestJson(
	    const ShaderCookPackageDesc& package,
	    const ShaderCookStageDesc& stage,
	    const ShaderCompileOptions& options,
	    const CookedStageBuild& compiledStage);
	static std::string BuildCacheInfoJson(const ShaderCompileOptions& options, const CookedStageBuild& compiledStage);
	static std::string BuildReflectionJson(const ShaderReflection& reflection);
};
