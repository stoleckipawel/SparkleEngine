#pragma once

#include "Rendering/RenderInputFrame.h"

#include <string>

class RenderWorld;

class RenderFrameDynamicDataValidator final
{
  public:
	static bool Validate(
	    const RenderWorld& world,
	    const RenderInputFrame& input,
	    std::string& diagnostic);
};
