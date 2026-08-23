#pragma once

#include "Cooking/ShaderCookContext.h"

class ShaderBackendPool;
struct ShaderCookSettings;

class ShaderCookPlanBuilder final
{
public:
	ShaderCookPlanBuilder() = delete;

	static ShaderCookPipelinePlan Build(const ShaderCookSettings& settings, ShaderBackendPool& backendPool);

private:
	static void BuildDependencyManifest(const ShaderCookSettings& settings, ShaderCookPipelinePlan& plan);
};
