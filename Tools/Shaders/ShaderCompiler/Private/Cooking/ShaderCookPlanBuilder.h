#pragma once

#include "Cooking/ShaderCookContext.h"
#include "ToolWorkProgressEvent.h"

class ShaderBackendPool;
struct ShaderCookSettings;

class ShaderCookPlanBuilder final
{
public:
	ShaderCookPlanBuilder() = delete;

	static ShaderCookPipelinePlan Build(
	    const ShaderCookSettings& settings,
	    ShaderBackendPool& backendPool,
	    const ToolWorkProgressCallback& progress);

private:
	static void BuildDependencyManifest(ShaderCookPipelinePlan& plan);
};
