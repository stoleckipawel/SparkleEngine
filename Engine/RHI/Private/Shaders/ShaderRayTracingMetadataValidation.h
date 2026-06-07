#pragma once

#include "Shaders/CookedShaderPackage.h"

#include <string>

class LoadedShaderPackage;

namespace ShaderRayTracingMetadataValidation
{
	bool ValidateInlineRayQueryMetadata(
	    const LoadedShaderPackage& package,
	    CookedShaderBinaryFormat requiredBinaryFormat,
	    std::string& outErrorMessage);
}
