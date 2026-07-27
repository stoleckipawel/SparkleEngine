#pragma once

#include "RHI/Public/ShaderParameters/PassParameterLayout.h"
#include "RHI/Public/Shaders/CookedShaderPackageIdentity.h"
#include "Shaders/Authoring/ShaderParameterStruct.h"

#include <filesystem>
#include <optional>
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
	std::optional<ShaderParameterStructDescriptor> parameterStructDescriptor;
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
	CookedShaderPackageKind packageKind = CookedShaderPackageKind::Graphics;
	CookedShaderPackageFeatureFlags packageFeatures = CookedShaderPackageFeatureFlags::None;
	std::uint32_t rayTracingPayloadSizeInBytes = 0;
	std::uint32_t rayTracingAttributeSizeInBytes = 0;
	std::uint32_t rayTracingMaxRecursionDepth = 0;
	std::vector<ShaderCookStageDesc> stages;
	std::vector<ShaderCookRayTracingExportDesc> rayTracingExports;
	std::vector<ShaderCookRayTracingHitGroupDesc> rayTracingHitGroups;
};
