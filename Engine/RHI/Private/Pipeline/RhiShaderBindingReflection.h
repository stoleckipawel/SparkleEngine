#pragma once

#include "Pipeline/RhiPipelineDesc.h"
#include "Shaders/CookedShaderPackage.h"

#include <vector>

class LoadedShaderPackage;
class PassParameterLayout;

struct RhiReflectedBindingLocation final
{
	RhiBindingPoint BindingPoint = {};
	ShaderStageMask VisibilityMask = ShaderStageMask::None;
};

class RhiShaderBindingReflection final
{
  public:
	static std::vector<RhiReflectedBindingLocation> ResolveLocations(
	    const LoadedShaderPackage& shaderPackage,
	    const PassParameterLayout& parameterLayout,
	    const CookedShaderBindingRecord& bindingRecord,
	    CookedShaderBinaryFormat binaryFormat);
};
