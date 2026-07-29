#pragma once

#include "Shaders/CookedShaderPackage.h"

class LoadedShaderPackage;

namespace ShaderRayTracingMetadataValidation
{
	void ValidateInlineRayQueryMetadata(
	    const LoadedShaderPackage& package,
	    CookedShaderBinaryFormat runtimeBinaryFormat);
}
