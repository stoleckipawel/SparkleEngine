#pragma once

#include "Manifest/ShaderCookManifestTypes.h"
#include "RHI/Public/Shaders/ShaderCompileOptions.h"

class ShaderCompileOptionsBuilder final
{
  public:
	static ShaderCompileOptions Build(const ShaderCookStageDesc& stage);
};
