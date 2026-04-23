#pragma once

#include "Cooking/Cache/ShaderCacheKey.h"
#include "Cooking/CookedStageBuild.h"

#include <string>

class IShaderArtifactStore
{
  public:
	virtual ~IShaderArtifactStore() = default;
	virtual bool TryGet(const ShaderCacheKey& key, CookedStageBuild& outBuild, std::string& outErrorMessage) const = 0;
	virtual bool Put(const ShaderCacheKey& key, const CookedStageBuild& build, std::string& outErrorMessage) = 0;
};
