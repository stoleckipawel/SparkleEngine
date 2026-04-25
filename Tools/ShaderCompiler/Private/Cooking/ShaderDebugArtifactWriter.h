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
	static bool Write(
	    const std::filesystem::path& rootDirectory,
	    const ShaderCookPackageDesc& package,
	    const ShaderCookStageDesc& stage,
	    const ShaderCompileOptions& options,
	    const CookedStageBuild& compiledStage,
	    const ShaderDebugArtifactSet& debugArtifacts,
	    std::string& outErrorMessage);

  private:
	static std::string BuildBundleDirectoryName(
	    const ShaderCookPackageDesc& package,
	    const ShaderCookStageDesc& stage,
	    const ShaderCompileOptions& options,
	    const CookedStageBuild& compiledStage);
	static std::string SanitizePathComponent(std::string_view value);
	static std::string EscapeJson(std::string_view value);
	static std::string BuildCompileRequestJson(
	    const ShaderCookPackageDesc& package,
	    const ShaderCookStageDesc& stage,
	    const ShaderCompileOptions& options,
	    const CookedStageBuild& compiledStage);
	static std::string BuildDefinesJson(const ShaderCompileOptions& options);
	static std::string BuildPermutationJson(const ShaderCookPackageDesc& package);
	static std::string BuildCompileArgsJson(const ShaderDebugArtifactSet& debugArtifacts);
	static std::string BuildReflectionJson(const ShaderReflection& reflection);
	static std::string BuildParameterMatchJson();
	static bool WriteTextFile(
	    const std::filesystem::path& path,
	    std::string_view text,
	    std::string& outErrorMessage);
};