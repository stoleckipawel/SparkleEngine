#pragma once

#include "RHI/Public/Shaders/CookedShaderPackage.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

struct InspectedCookedShaderBinary final
{
	std::uint64_t shaderBlobId = 0;
	ShaderStage stage = ShaderStage::Count;
	CookedShaderBinaryFormat format = CookedShaderBinaryFormat::Dxil;
	std::string entryPoint;
	std::string exportName;
	std::string backendName;
	std::string codegenTarget;
	std::uint64_t bytecodeHash = 0;
	std::uint64_t backendVersion = 0;
	std::uint32_t bytecodeSizeInBytes = 0;
	std::uint32_t resourceBindingCount = 0;
	std::uint32_t constantBufferCount = 0;
	std::uint32_t inputElementCount = 0;
	std::uint32_t pushConstantRangeCount = 0;
	std::uint32_t specializationConstantCount = 0;
};

struct InspectedCookedPipelineLayout final
{
	std::string codegenTarget;
	std::uint64_t bindingLayoutHash = 0;
	std::uint32_t bindingRecordCount = 0;
	std::uint32_t descriptorBindingCount = 0;
	std::uint32_t descriptorSetCount = 0;
	std::uint32_t pushConstantRangeCount = 0;
	std::uint32_t pushConstantSizeInBytes = 0;
	std::uint32_t constantBufferCount = 0;
	std::uint32_t readOnlyResourceCount = 0;
	std::uint32_t readWriteResourceCount = 0;
	std::uint32_t samplerCount = 0;
	std::uint32_t accelerationStructureCount = 0;
};

struct InspectedCookedRayTracingExport final
{
	CookedShaderRayTracingExportKind kind = CookedShaderRayTracingExportKind::None;
	std::string exportName;
	std::string entryPoint;
	std::uint32_t binaryRecordIndex = UINT32_MAX;
	std::uint64_t exportHash = 0;
};

struct InspectedCookedRayTracingHitGroup final
{
	CookedShaderRayTracingHitGroupType type = CookedShaderRayTracingHitGroupType::Triangles;
	std::string name;
	std::uint32_t closestHitExportIndex = UINT32_MAX;
	std::uint32_t anyHitExportIndex = UINT32_MAX;
	std::uint32_t intersectionExportIndex = UINT32_MAX;
	std::uint64_t hitGroupHash = 0;
};

struct InspectedCookedAccelerationStructureBinding final
{
	std::string name;
	std::uint32_t set = 0;
	std::uint32_t slot = 0;
	std::uint32_t arrayCount = 1;
};

struct InspectedCookedShaderPackage final
{
	std::uint64_t packageKey = 0;
	std::uint64_t sourceIdentityHash = 0;
	std::uint64_t bindingLayoutHash = 0;
	CookedShaderPackageKind packageKind = CookedShaderPackageKind::Graphics;
	CookedShaderPackageFeatureFlags packageFeatures = CookedShaderPackageFeatureFlags::None;
	std::uint32_t rayTracingPayloadSizeInBytes = 0;
	std::uint32_t rayTracingAttributeSizeInBytes = 0;
	std::uint32_t rayTracingMaxRecursionDepth = 0;
	std::uint32_t binaryRecordCount = 0;
	std::uint32_t pipelineLayoutRecordCount = 0;
	std::uint32_t reflectionRecordCount = 0;
	std::uint32_t rayTracingLocalParameterRecordCount = 0;
	std::vector<InspectedCookedShaderBinary> binaries;
	std::vector<InspectedCookedPipelineLayout> pipelineLayouts;
	std::vector<InspectedCookedRayTracingExport> rayTracingExports;
	std::vector<InspectedCookedRayTracingHitGroup> rayTracingHitGroups;
	std::vector<InspectedCookedAccelerationStructureBinding> accelerationStructureBindings;
};

class CookedPackageInspection final
{
  public:
	CookedPackageInspection() = delete;

	static InspectedCookedShaderPackage Inspect(const std::filesystem::path& packagePath);
	static const char* GetBinaryFormatName(CookedShaderBinaryFormat format) noexcept;
	static const char* GetPackageKindName(CookedShaderPackageKind kind) noexcept;
	static const char* GetRayTracingExportKindName(CookedShaderRayTracingExportKind kind) noexcept;
	static const char* GetRayTracingHitGroupTypeName(CookedShaderRayTracingHitGroupType type) noexcept;
	static std::string FormatPackageFeatures(CookedShaderPackageFeatureFlags features);
};
