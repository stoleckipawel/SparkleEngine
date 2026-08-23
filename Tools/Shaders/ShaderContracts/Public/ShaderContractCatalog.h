#pragma once

#include "RHI/Public/Shaders/Authoring/ShaderParameterStruct.h"
#include "RHI/Public/Shaders/CookedShaderPackage.h"
#include "RHI/Public/Shaders/ShaderStage.h"
#include "RHI/Public/ShaderParameters/PassParameterLayout.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

struct ShaderContractStage final
{
	std::string shaderName;
	std::string packageId;
	std::string bindingLayoutId;
	std::filesystem::path sourcePath;
	std::string entryPoint;
	ShaderStage stage = ShaderStage::Count;
	CookedShaderPackageKind packageKind = CookedShaderPackageKind::Graphics;
	CookedShaderPackageFeatureFlags packageFeatures = CookedShaderPackageFeatureFlags::None;
	CookedShaderRayTracingExportKind rayTracingExportKind = CookedShaderRayTracingExportKind::None;
	std::string rayTracingExportName;
	std::uint32_t rayTracingPayloadSizeInBytes = 0;
	std::uint32_t rayTracingAttributeSizeInBytes = 0;
	std::uint32_t rayTracingMaxRecursionDepth = 0;
	ShaderParameterStructDescriptor parameterStruct;
	bool hasParameterStruct = false;
};

struct ShaderContractRayTracingHitGroup final
{
	std::string packageId;
	std::string name;
	std::string closestHitExportName;
	std::string anyHitExportName;
	std::string intersectionExportName;
};

struct ShaderContractPackage final
{
	std::string packageId;
	std::string bindingLayoutId;
	PassParameterLayout bindingLayout;
	CookedShaderPackageKind packageKind = CookedShaderPackageKind::Graphics;
	CookedShaderPackageFeatureFlags packageFeatures = CookedShaderPackageFeatureFlags::None;
	std::uint32_t rayTracingPayloadSizeInBytes = 0;
	std::uint32_t rayTracingAttributeSizeInBytes = 0;
	std::uint32_t rayTracingMaxRecursionDepth = 0;
	std::vector<ShaderContractStage> stages;
	std::vector<ShaderContractRayTracingHitGroup> rayTracingHitGroups;
};

struct ShaderContractCatalog final
{
	std::vector<ShaderContractPackage> packages;
	std::vector<ShaderContractStage> stages;
};

struct ShaderContractVerificationFailure final
{
	std::string shaderName;
	std::string packageId;
	std::string bindingLayoutId;
	std::filesystem::path sourcePath;
	std::string entryPoint;
	ShaderStage stage = ShaderStage::Count;
	std::string parameterName;
	std::string expectedLayout;
	std::string reason;
};

struct ShaderContractJobIdentity final
{
	std::string packageId;
	std::filesystem::path sourcePath;
	std::string entryPoint;
	ShaderStage stage = ShaderStage::Count;
	std::string backendName;
	std::string targetName;
	std::string profileName;
	std::uint64_t sourceHash = 0;
	std::uint64_t includeClosureHash = 0;
	std::uint64_t optionsHash = 0;
	std::uint64_t compileInputHash = 0;
};
