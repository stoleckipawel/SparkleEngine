#pragma once

#include "RHI/Public/ShaderParameters/PassParameterLayout.h"
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
};

struct ShaderCookRayTracingExportDesc final
{
	std::string shaderName;
	CookedShaderRayTracingExportKind kind = CookedShaderRayTracingExportKind::None;
	std::string exportName;
	std::string entryPoint;
	std::uint32_t stageIndex = 0;
};

struct ShaderCookRayTracingHitGroupDesc final
{
	std::string name;
	std::string closestHitShaderName;
	std::string anyHitShaderName;
	std::string intersectionShaderName;
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