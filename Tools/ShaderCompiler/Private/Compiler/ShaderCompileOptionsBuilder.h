#pragma once

#include "Manifest/ShaderCookManifestTypes.h"
#include "ShaderCompileOptions.h"

class ShaderCompileOptionsBuilder final
{
  public:
	static ShaderCompileOptions Build(const ShaderCookStageDesc& stage);
};
