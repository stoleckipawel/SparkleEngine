#pragma once

#include "Cooking/ShaderCookContext.h"
#include "Cooking/ShaderCookProgress.h"

class ShaderBackendPool;
struct ShaderCookSettings;

class ShaderCookPlanBuilder final
{
public:
	ShaderCookPlanBuilder() = delete;

	static ShaderCookPipelinePlan Build(
	    const ShaderCookSettings& settings,
	    ShaderBackendPool& backendPool,
	    const ShaderCookProgressCallback& progress);

private:
	static void BuildDependencyManifest(ShaderCookPipelinePlan& plan);
};
