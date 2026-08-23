#pragma once

#include "RHI/Public/Shaders/CookedShaderPackage.h"
#include "ShaderReflection.h"

#include <cstdint>
#include <string>
#include <vector>

struct CookedStageBuild final
{
	ShaderStage stage = ShaderStage::Count;
	CookedShaderBinaryFormat format = CookedShaderBinaryFormat::Dxil;
	std::string sourcePath;
	std::string entryPoint;
	std::string debugArtifact;
	std::string backendName;
	std::string codegenTarget;
	std::uint64_t shaderBlobId = 0;
	std::uint64_t backendVersion = 0;
	std::uint64_t sourceHash = 0;
	std::uint64_t includeClosureHash = 0;
	std::uint64_t requestHash = 0;
	std::uint64_t compileInputHash = 0;
	std::vector<std::uint8_t> bytecode;
	std::uint64_t bytecodeHash = 0;
	ShaderReflection reflection;
};
