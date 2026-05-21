#include "PCH.h"

#include "Cooking/ShaderCookPlanner.h"

#include "Core/Public/Assets/AssetTypes.h"
#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Paths/DirectoryPaths.h"
#include "Core/Public/Paths/PathUtils.h"
#include "Shaders/Authoring/GlobalShader.h"
#include "Shaders/ShaderPackageLayoutBuilder.h"

#include <algorithm>
#include <format>
#include <unordered_map>

ShaderCompileOptions ShaderCookPlanner::BuildCompileOptions(const ShaderCookStageDesc& stage)
{
	ShaderCompileOptions options{};
	options.SourcePath = Filesystem::ResolveAssetPathValidated(stage.sourcePath, AssetType::Shader);
	options.EntryPoint = stage.entryPoint;
	options.Stage = stage.stage;
	options.PackageKind = stage.packageKind;
	options.PackageFeatures = stage.packageFeatures;
	options.RayTracingExportKind = stage.rayTracingExportKind;

	const std::filesystem::path& projectShaderRoot = Paths::ShaderSourceRoot(PathRoot::Project);
	const std::filesystem::path& engineShaderRoot = Paths::ShaderSourceRoot(PathRoot::Engine);

	if (!projectShaderRoot.empty())
	{
		options.IncludeDir = projectShaderRoot;
		if (!engineShaderRoot.empty() &&
		    Paths::MakePathKey(engineShaderRoot) != Paths::MakePathKey(projectShaderRoot))
		{
			options.AdditionalIncludeDirs.push_back(engineShaderRoot);
		}
	}
	else
	{
		options.IncludeDir = engineShaderRoot;
	}

#if defined(ENGINE_SHADERS_DEBUG)
	options.EnableDebugInfo = true;
	options.StripDebugInfo = false;
#endif

	options.EnableOptimizations = true;
	return options;
}

std::vector<ShaderCookPackageDesc> ShaderCookPlanner::BuildPackages(
	const ShaderPackageCookSettings& settings,
	std::string& outErrorMessage)
{
	if (!settings.packageId.empty() && !settings.shaderId.empty())
	{
		outErrorMessage = "Use either package id or shader id for a shader cook request, not both.";
		return {};
	}

	CookSelectionKind selectionKind = CookSelectionKind::All;
	std::string_view requestedId;
	if (!settings.packageId.empty())
	{
		selectionKind = CookSelectionKind::PackageId;
		requestedId = settings.packageId;
	}
	else if (!settings.shaderId.empty())
	{
		selectionKind = CookSelectionKind::ShaderId;
		requestedId = settings.shaderId;
	}

	std::vector<ShaderCookPackageDesc> packages = BuildTypedShaderPackages(selectionKind, requestedId, outErrorMessage);
	if (!outErrorMessage.empty())
	{
		return {};
	}

	if (packages.empty())
	{
		outErrorMessage = "No typed shader registrations were found.";
		return {};
	}

	outErrorMessage.clear();
	return packages;
}

std::optional<ShaderParameterStructDescriptor> ShaderCookPlanner::FindParameterStructDescriptor(
	const ShaderCompileOptions& options)
{
	const std::string sourcePath = options.SourcePath.generic_string();
	for (const ShaderRegistrationDesc& shader : GlobalShaderRegistry::GetRegistrations())
	{
		const std::string registeredSourcePath(shader.SourcePath);
		const bool sourceMatches = sourcePath == registeredSourcePath || sourcePath.ends_with("/" + registeredSourcePath);
		if (shader.Stage != options.Stage || shader.PackageKind != options.PackageKind || shader.EntryPoint != options.EntryPoint || !sourceMatches)
		{
			continue;
		}

		if (shader.BuildParameterStructDescriptor == nullptr)
		{
			return ShaderParameterStructDescriptor{};
		}
		return shader.BuildParameterStructDescriptor();
	}

	return std::nullopt;
}

std::vector<ShaderCookPackageDesc> ShaderCookPlanner::BuildTypedShaderPackages(
	CookSelectionKind selectionKind,
	std::string_view requestedId,
	std::string& outErrorMessage)
{
	std::vector<ShaderCookPackageDesc> packages;
	std::unordered_map<std::string, std::size_t> packageIndices;
	const std::string requested(requestedId);
	std::vector<const ShaderRegistrationDesc*> selectedShaders;

	for (const ShaderRegistrationDesc& shader : GlobalShaderRegistry::GetRegistrations())
	{
		const std::string packageId = GetShaderRegistrationPackageId(shader);
		const std::string shaderName(shader.ShaderName);
		const bool selected =
		    selectionKind == CookSelectionKind::All ||
		    (selectionKind == CookSelectionKind::PackageId && packageId == requested) ||
		    (selectionKind == CookSelectionKind::ShaderId && shaderName == requested);
		if (!selected)
		{
			continue;
		}

		selectedShaders.push_back(&shader);
	}

	for (const ShaderRegistrationDesc* shaderPtr : selectedShaders)
	{
		const ShaderRegistrationDesc& shader = *shaderPtr;
		const std::string packageId = GetShaderRegistrationPackageId(shader);
		const std::string rayTracingExportName =
		    shader.RayTracingExportName.empty() ? std::string(shader.EntryPoint) : std::string(shader.RayTracingExportName);
		const std::string bindingLayoutId(GetShaderRegistrationBindingLayoutId(shader));
		auto packageIt = packageIndices.find(packageId);
		if (packageIt == packageIndices.end())
		{
			PassParameterLayout bindingLayout;
			if (!ShaderPackageLayoutBuilder::Build(packageId, GlobalShaderRegistry::GetRegistrations(), bindingLayout, outErrorMessage))
			{
				return {};
			}

			ShaderCookPackageDesc package;
			package.packageId = packageId;
			package.bindingLayoutId = bindingLayoutId;
			package.bindingLayout = std::move(bindingLayout);
			package.packageKind = shader.PackageKind;
			package.packageFeatures = shader.PackageFeatures;
			package.rayTracingPayloadSizeInBytes = shader.RayTracingPayloadSizeInBytes;
			package.rayTracingAttributeSizeInBytes = shader.RayTracingAttributeSizeInBytes;
			package.rayTracingMaxRecursionDepth = shader.RayTracingMaxRecursionDepth;
			packages.push_back(std::move(package));
			packageIt = packageIndices.emplace(packageId, packages.size() - 1).first;
		}

		ShaderCookPackageDesc& package = packages[packageIt->second];
		if (package.bindingLayoutId != bindingLayoutId)
		{
			outErrorMessage = std::format(
			    "Typed shader package '{}' mixes binding layouts '{}' and '{}'",
			    packageId,
			    package.bindingLayoutId,
			    bindingLayoutId);
			return {};
		}
		if (package.packageKind != shader.PackageKind)
		{
			outErrorMessage = std::format(
			    "Typed shader package '{}' mixes package kinds {} and {}",
			    packageId,
			    static_cast<std::uint32_t>(package.packageKind),
			    static_cast<std::uint32_t>(shader.PackageKind));
			return {};
		}
		package.packageFeatures |= shader.PackageFeatures;
		package.rayTracingPayloadSizeInBytes = std::max(package.rayTracingPayloadSizeInBytes, shader.RayTracingPayloadSizeInBytes);
		package.rayTracingAttributeSizeInBytes = std::max(package.rayTracingAttributeSizeInBytes, shader.RayTracingAttributeSizeInBytes);
		package.rayTracingMaxRecursionDepth = std::max(package.rayTracingMaxRecursionDepth, shader.RayTracingMaxRecursionDepth);

		const std::uint32_t stageIndex = static_cast<std::uint32_t>(package.stages.size());
		package.stages.push_back(ShaderCookStageDesc{
		    .stage = shader.Stage,
		    .sourcePath = std::filesystem::path(std::string(shader.SourcePath)),
		    .entryPoint = std::string(shader.EntryPoint),
		    .packageKind = shader.PackageKind,
		    .packageFeatures = shader.PackageFeatures,
		    .rayTracingExportKind = shader.RayTracingExportKind,
		    .rayTracingExportName = rayTracingExportName});

		if (shader.PackageKind == CookedShaderPackageKind::RayTracingLibrary)
		{
			package.rayTracingExports.push_back(ShaderCookRayTracingExportDesc{
			    .exportLookupName = rayTracingExportName,
			    .kind = shader.RayTracingExportKind,
			    .exportName = rayTracingExportName,
			    .entryPoint = std::string(shader.EntryPoint),
			    .stageIndex = stageIndex});
		}
	}

	for (const RayTracingHitGroupRegistrationDesc& hitGroup : GlobalShaderRegistry::GetRayTracingHitGroups())
	{
		const std::string packageId(hitGroup.PackageName);
		const bool wholePackageSelected = selectionKind == CookSelectionKind::All ||
		    (selectionKind == CookSelectionKind::PackageId && packageId == requested);
		if (!wholePackageSelected)
		{
			continue;
		}

		for (ShaderCookPackageDesc& package : packages)
		{
			if (package.packageId != packageId)
			{
				continue;
			}
			if (package.packageKind != CookedShaderPackageKind::RayTracingLibrary)
			{
				outErrorMessage = std::format("Ray tracing hit group '{}' targets non-RT shader package '{}'", hitGroup.HitGroupName, packageId);
				return {};
			}

			package.rayTracingHitGroups.push_back(ShaderCookRayTracingHitGroupDesc{
			    .name = std::string(hitGroup.HitGroupName),
			    .closestHitExportName = std::string(hitGroup.ClosestHitExportName),
			    .anyHitExportName = std::string(hitGroup.AnyHitExportName),
			    .intersectionExportName = std::string(hitGroup.IntersectionExportName)});
		}
	}

	if (selectionKind != CookSelectionKind::All && packages.empty())
	{
		outErrorMessage = selectionKind == CookSelectionKind::PackageId
		    ? "Unknown typed shader package '" + requested + "'"
		    : "Unknown registered shader id '" + requested + "'";
		return {};
	}

	outErrorMessage.clear();
	return packages;
}