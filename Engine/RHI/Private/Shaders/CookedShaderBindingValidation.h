#pragma once

#include "ShaderParameters/PassParameterLayout.h"
#include "Shaders/CookedShaderPackageIdentity.h"
#include "Shaders/LoadedShaderPackage.h"

#include <string>
#include <vector>

namespace CookedShaderBindingValidation
{
	bool Validate(
	    const LoadedShaderPackage& package,
	    const ShaderPackageDefinition& definition,
	    const std::vector<PassParameterDesc>& expectedParameters,
	    CookedShaderBinaryFormat requiredBinaryFormat,
	    std::string& outErrorMessage);
}
