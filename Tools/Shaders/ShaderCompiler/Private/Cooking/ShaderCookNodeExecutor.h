#pragma once

#include "Cooking/CookNode.h"
#include "Cooking/CookedStageBuild.h"

#include <filesystem>
#include <optional>
#include <string_view>

class IShaderArtifactStore;
class IShaderBackend;
struct ShaderDebugArtifactSet;
struct ShaderPackageCookSettings;

class ShaderCookNodeExecutor final
{
  public:
	ShaderCookNodeExecutor() = delete;
	static CookedStageBuild Execute(
	    const ShaderPackageCookSettings& settings,
	    const CookNode& node,
	    const std::filesystem::path& cacheDirectory);

  private:
	static std::optional<CookedStageBuild> LoadFromCache(
	    const ShaderPackageCookSettings& settings,
	    const CookNode& node,
	    IShaderArtifactStore& artifactStore);
	static CookedStageBuild Compile(
	    const ShaderPackageCookSettings& settings,
	    const CookNode& node,
	    IShaderBackend& backend,
	    IShaderArtifactStore& artifactStore);
	static void PublishArtifacts(
	    const ShaderPackageCookSettings& settings,
	    const CookNode& node,
	    IShaderArtifactStore& artifactStore,
	    const ShaderDebugArtifactSet& debugArtifacts,
	    const CookedStageBuild& compiledStage);
	static void ApplyNodeMetadata(const CookNode& node, std::string_view cacheStatus, CookedStageBuild& compiledStage);
};
