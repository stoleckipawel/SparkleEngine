#pragma once

#include "Backend/ShaderTarget.h"
#include "Cooking/CookNode.h"

#include <string>
#include <string_view>

class ShaderCookDiagnostics final
{
  public:
	ShaderCookDiagnostics() = delete;

	static std::string FormatNodeContext(
	    const CookNode& node,
	    std::string_view backendName,
	    ShaderTarget target);
};