#pragma once

#include "ShaderParameters/PassParameterLayout.h"
#include "Shaders/CookedShaderPackageIdentity.h"
#include "Shaders/LoadedShaderPackage.h"

#include <string>
#include <vector>

namespace CookedShaderBindingDiagnostics
{
	const char* FormatResourceKind(CookedShaderResourceKind kind) noexcept;

	std::string Append(
	    std::string message,
	    const LoadedShaderPackage& package,
	    const ShaderPackageDefinition& definition,
	    const std::vector<PassParameterDesc>& expectedParameters,
	    CookedShaderBinaryFormat requiredBinaryFormat);
}
