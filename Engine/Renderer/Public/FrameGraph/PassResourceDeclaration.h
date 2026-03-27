#pragma once

#include "Renderer/Public/FrameGraph/ResourceHandle.h"
#include "Renderer/Public/FrameGraph/ResourceUsage.h"

#include <string>

struct PassResourceDeclaration
{
	ResourceHandle handle = ResourceHandle::Invalid();
	ResourceUsage usage = ResourceUsage::ShaderRead;
	std::string label;
};