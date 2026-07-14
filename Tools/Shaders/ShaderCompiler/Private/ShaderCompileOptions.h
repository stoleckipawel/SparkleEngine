#pragma once

#include "Backend/ShaderTarget.h"
#include "RHI/Public/Shaders/CookedShaderPackage.h"
#include "RHI/Public/Shaders/ShaderStage.h"

#include <filesystem>
#include <string>
#include <vector>

struct ShaderDescriptorBindingRemap final
{
	std::string Name;
	std::uint32_t Set = 0;
	std::uint32_t Binding = 0;
};

// Inputs to one shader compile invocation. Owned by the offline tool; runtime
// modules must not include this header.
struct ShaderCompileOptions
{
	std::filesystem::path SourcePath;
	std::filesystem::path IncludeDir;
	std::string EntryPoint = "main";
	ShaderStage Stage = ShaderStage::Pixel;
	ShaderTarget Target = kDefaultShaderTarget;
	CookedShaderPackageKind PackageKind = CookedShaderPackageKind::Graphics;
	CookedShaderPackageFeatureFlags PackageFeatures = CookedShaderPackageFeatureFlags::None;
	CookedShaderRayTracingExportKind RayTracingExportKind = CookedShaderRayTracingExportKind::None;

	bool EnableDebugInfo = false;
	bool EnableOptimizations = true;
	bool TreatWarningsAsErrors = true;
	bool StripDebugInfo = true;
	bool CaptureDebugArtifacts = false;

	std::vector<std::filesystem::path> AdditionalIncludeDirs;
	std::vector<std::string> Defines;
	std::vector<ShaderDescriptorBindingRemap> DescriptorBindingRemaps;
};
