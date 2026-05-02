#pragma once

#include "RHI/Public/ShaderParameters/PassParameterLayout.h"
#include "RHI/Public/Shaders/Authoring/ShaderPermutation.h"
#include "RHI/Public/Shaders/CookedShaderPackageUtils.h"

#include <filesystem>
#include <string>
#include <vector>

struct ShaderCookStageDesc final
{
	ShaderStage stage = ShaderStage::Count;
	std::filesystem::path sourcePath;
	std::string entryPoint;
	CookedShaderPackageKind packageKind = CookedShaderPackageKind::Graphics;
	CookedShaderPackageFeatureFlags packageFeatures = CookedShaderPackageFeatureFlags::None;
	CookedShaderRayTracingExportKind rayTracingExportKind = CookedShaderRayTracingExportKind::None;
	std::string rayTracingExportName;
	ShaderPermutationDomainDescriptor permutationDomain;
	ShaderPermutationVector permutationVector;
	ShaderPermutationKey permutationKey = 0;
	std::string permutationVectorName;
};

struct ShaderCookRayTracingExportDesc final
{
	std::string exportLookupName;
	CookedShaderRayTracingExportKind kind = CookedShaderRayTracingExportKind::None;
	std::string exportName;
	std::string entryPoint;
	std::uint32_t stageIndex = 0;
};

struct ShaderCookRayTracingHitGroupDesc final
{
	std::string name;
	std::string closestHitExportName;
	std::string anyHitExportName;
	std::string intersectionExportName;
};

struct ShaderCookPackageDesc final
{
	std::string packageId;
	std::string bindingLayoutId;
	PassParameterLayout bindingLayout;
	std::string variantId = "Default";
	CookedShaderPackageKind packageKind = CookedShaderPackageKind::Graphics;
	CookedShaderPackageFeatureFlags packageFeatures = CookedShaderPackageFeatureFlags::None;
	std::uint32_t rayTracingPayloadSizeInBytes = 0;
	std::uint32_t rayTracingAttributeSizeInBytes = 0;
	std::uint32_t rayTracingMaxRecursionDepth = 0;
	std::vector<ShaderCookStageDesc> stages;
	std::vector<ShaderCookRayTracingExportDesc> rayTracingExports;
	std::vector<ShaderCookRayTracingHitGroupDesc> rayTracingHitGroups;
};