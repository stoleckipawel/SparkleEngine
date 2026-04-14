#pragma once

#include "ResourceHandle.h"
#include "ResourceUsage.h"

#include <string>

struct PassResourceDeclaration
{
	ResourceHandle handle = ResourceHandle::Invalid();
	ResourceUsage usage = ResourceUsage::ShaderRead;
	std::string label;
};