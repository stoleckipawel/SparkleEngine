#pragma once

#include "Compiler/ShaderCompileRequest.h"

#include <string>

class ShaderCompileProfile final
{
public:
	ShaderCompileProfile() = delete;

	// Fixed SPIR-V ABI shared by compile identity and every backend's native option mapping.
	static constexpr bool SpirVUseDirectXBufferLayout = true;
	static constexpr bool SpirVUseUnknownStorageImageFormat = true;

	static const char* GetShaderModelProfileName(ShaderTarget target);
	static const char* GetSpirVProfileName(ShaderTarget target);
	static const char* GetSlangTargetProfileName(ShaderTarget target);
	static std::string BuildTargetProfile(const ShaderCompileRequest& request);
};
