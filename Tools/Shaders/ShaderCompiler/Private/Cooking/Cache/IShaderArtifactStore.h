#pragma once

#include "Cooking/Cache/ShaderCacheKey.h"
#include "Cooking/CookedStageBuild.h"

#include <optional>

class IShaderArtifactStore
{
  public:
	virtual ~IShaderArtifactStore() = default;
	virtual std::optional<CookedStageBuild> Find(const ShaderCacheKey& key) const = 0;
	virtual void Put(const ShaderCacheKey& key, const CookedStageBuild& build) = 0;
};
