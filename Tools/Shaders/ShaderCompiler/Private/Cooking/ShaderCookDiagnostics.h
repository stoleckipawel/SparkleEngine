#pragma once

#include "Backend/ShaderTarget.h"
#include "Cooking/ShaderCompileJob.h"

#include <string>
#include <string_view>

class ShaderCookDiagnostics final
{
public:
	ShaderCookDiagnostics() = delete;

	static std::string FormatJobContext(const ShaderCompileJob& job, std::string_view backendName, ShaderTarget target);
};
