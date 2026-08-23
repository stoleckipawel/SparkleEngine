#pragma once

#include "Compiler/ShaderCompileRequest.h"
#include "Cooking/CookedStageBuild.h"
#include "ShaderDebugArtifactSet.h"

#include <filesystem>
#include <string>

class ShaderDebugArtifactWriter final
{
public:
	static void Write(
	    const std::filesystem::path& rootDirectory,
	    const ShaderCompileRequest& request,
	    const CookedStageBuild& compiledStage,
	    const ShaderDebugArtifactSet& debugArtifacts);

private:
	static void WriteText(const std::filesystem::path& path, std::string_view contents);
	static void WriteCompileInputs(
	    const std::filesystem::path& bundleDirectory,
	    const ShaderCompileRequest& request,
	    const CookedStageBuild& compiledStage);
	static void WriteCompilerOutputs(
	    const std::filesystem::path& bundleDirectory,
	    const ShaderDebugArtifactSet& debugArtifacts,
	    const CookedStageBuild& compiledStage);
	static std::string BuildBundleDirectoryName(const ShaderCompileRequest& request, const CookedStageBuild& compiledStage);
	static std::string BuildCompileRequestJson(const ShaderCompileRequest& request, const CookedStageBuild& compiledStage);
	static std::string BuildCompileIdentityJson(const ShaderCompileRequest& request, const CookedStageBuild& compiledStage);
	static std::string BuildReflectionJson(const ShaderReflection& reflection);
};
