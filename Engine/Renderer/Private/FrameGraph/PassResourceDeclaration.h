#pragma once

#include "FrameGraph/FrameGraphResourceHandle.h"
#include "ResourceUsage.h"

#include <string>

struct PassResourceDeclaration
{
	FrameGraphResourceHandle handle = FrameGraphResourceHandle::Invalid();
	ResourceUsage usage = ResourceUsage::ShaderRead;
	std::string label;
};
