#pragma once

#include "Cooking/ShaderCookContext.h"
#include "Cooking/ShaderCookOutput.h"

#include <filesystem>

class ShaderArtifactPublication final
{
public:
	ShaderArtifactPublication() = delete;

	static ShaderCookOutput Publish(
	    const ShaderCookPipelinePlan& plan,
	    const std::filesystem::path& outputDirectory,
	    bool replaceCompleteCatalog);
};
