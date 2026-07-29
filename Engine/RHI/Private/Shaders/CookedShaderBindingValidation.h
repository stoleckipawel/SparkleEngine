#pragma once

#include "ShaderParameters/PassParameterLayout.h"
#include "Shaders/CookedShaderPackageIdentity.h"
#include "Shaders/LoadedShaderPackage.h"

#include <string>
#include <vector>

namespace CookedShaderBindingValidation
{
	void Validate(
	    const LoadedShaderPackage& package,
	    const ShaderPackageDefinition& definition,
	    const std::vector<PassParameterDesc>& expectedParameters,
	    CookedShaderBinaryFormat runtimeBinaryFormat);
}
