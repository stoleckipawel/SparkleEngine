#pragma once

#include "Cooking/CookNode.h"
#include "Cooking/ShaderCookNodeResult.h"

#include <filesystem>
#include <string>
#include <string_view>

class IShaderArtifactStore;
class IShaderBackend;
struct ShaderDebugArtifactSet;
struct ShaderPackageCookSettings;

class ShaderCookNodeExecutor final
{
  public:
	ShaderCookNodeExecutor() = delete;
	static void Execute(
	    const ShaderPackageCookSettings& settings,
	    const CookNode& node,
	    const std::filesystem::path& cacheDirectory,
	    ShaderCookNodeResult& outResult);

  private:
	static bool TryLoadFromCache(
	    const ShaderPackageCookSettings& settings,
	    const CookNode& node,
	    IShaderBackend& backend,
	    IShaderArtifactStore& artifactStore,
	    ShaderCookNodeResult& outResult);
	static bool Compile(
	    const ShaderPackageCookSettings& settings,
	    const CookNode& node,
	    IShaderBackend& backend,
	    IShaderArtifactStore& artifactStore,
	    ShaderCookNodeResult& outResult);
	static void ApplyNodeMetadata(const CookNode& node, std::string_view cacheStatus, CookedStageBuild& compiledStage);
};
