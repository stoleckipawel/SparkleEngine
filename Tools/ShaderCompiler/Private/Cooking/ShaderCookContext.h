#pragma once

#include "Cooking/CookedStageBuild.h"
#include "Cooking/Graph/DependencyGraph.h"
#include "Cooking/ShaderCookTypes.h"

#include <vector>

struct ShaderCookPackageContext final
{
	std::vector<CookedStageBuild> compiledStages;
};

struct ShaderCookPipelinePlan final
{
	std::vector<ShaderCookPackageDesc> packages;
	std::vector<ShaderCookPackageContext> packageContexts;
	DependencyGraph graph;
};