#pragma once

#include "RHI/Public/Shaders/CookedShaderPackage.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

struct InspectedCookedShaderBinary final
{
	ShaderStage stage = ShaderStage::Count;
	CookedShaderBinaryFormat format = CookedShaderBinaryFormat::Dxil;
	std::string entryPoint;
	std::string backendName;
	std::uint32_t bytecodeSizeInBytes = 0;
	std::uint32_t resourceBindingCount = 0;
	std::uint32_t constantBufferCount = 0;
	std::uint32_t inputElementCount = 0;
	std::uint32_t pushConstantRangeCount = 0;
	std::uint32_t specializationConstantCount = 0;
};

struct InspectedCookedShaderPackage final
{
	std::uint64_t packageKey = 0;
	std::uint32_t binaryRecordCount = 0;
	std::uint32_t reflectionRecordCount = 0;
	std::vector<InspectedCookedShaderBinary> binaries;
};

class CookedPackageInspection final
{
  public:
	CookedPackageInspection() = delete;

	static bool Inspect(
	    const std::filesystem::path& packagePath,
	    InspectedCookedShaderPackage& outPackage,
	    std::string& outErrorMessage);
	static const char* GetBinaryFormatName(CookedShaderBinaryFormat format) noexcept;
};