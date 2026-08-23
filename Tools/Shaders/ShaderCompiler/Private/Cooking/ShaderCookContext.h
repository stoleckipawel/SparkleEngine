#pragma once

#include "Cooking/CookedStageBuild.h"
#include "Cooking/Dependencies/ShaderDependencyManifest.h"
#include "Cooking/ShaderCompileJob.h"
#include "Cooking/ShaderCookTypes.h"

#include <vector>

using ShaderCookPackageContext = std::vector<CookedStageBuild>;

struct ShaderCookPipelinePlan final
{
	std::vector<ShaderCookPackageDesc> packages;
	std::vector<ShaderCookPackageContext> packageContexts;
	std::vector<ShaderCompileJob> jobs;
	std::vector<ShaderCompileConsumer> consumers;
	ShaderDependencyManifest dependencyManifest;
};
