#include "PCH.h"

#include "Cooking/ShaderCookPlanner.h"

#include "Core/Public/Assets/AssetTypes.h"
#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Paths/DirectoryPaths.h"
#include "Core/Public/Paths/PathUtils.h"
#include "Contracts/ShaderContractCatalogBuilder.h"

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
	std::string errorMessage;
	const ShaderContractCatalog catalog =
	    ShaderContractCatalogBuilder::Build(ShaderContractSelectionKind::All, {}, errorMessage);
	if (!errorMessage.empty())
	{
		return std::nullopt;
	}

	const std::string sourcePath = options.SourcePath.generic_string();
	for (const ShaderContractStage& stage : catalog.stages)
	{
		const std::string registeredSourcePath = stage.sourcePath.generic_string();
		const bool sourceMatches = sourcePath == registeredSourcePath || sourcePath.ends_with("/" + registeredSourcePath);
		if (stage.stage != options.Stage || stage.packageKind != options.PackageKind || stage.entryPoint != options.EntryPoint || !sourceMatches)
		{
			continue;
		}

		if (!stage.hasParameterStruct)
		{
			return ShaderParameterStructDescriptor{};
		}
		return stage.parameterStruct;
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
	const ShaderContractSelectionKind contractSelectionKind =
	    selectionKind == CookSelectionKind::PackageId ? ShaderContractSelectionKind::PackageId :
	    selectionKind == CookSelectionKind::ShaderId   ? ShaderContractSelectionKind::ShaderId :
	                                                    ShaderContractSelectionKind::All;
	const ShaderContractCatalog catalog =
	    ShaderContractCatalogBuilder::Build(contractSelectionKind, requested, outErrorMessage);
	if (!outErrorMessage.empty())
	{
		return {};
	}

	for (const ShaderContractPackage& contractPackage : catalog.packages)
	{
		const std::string& packageId = contractPackage.packageId;
		auto packageIt = packageIndices.find(packageId);
		if (packageIt == packageIndices.end())
		{
			ShaderCookPackageDesc package;
			package.packageId = packageId;
			package.bindingLayoutId = contractPackage.bindingLayoutId;
			package.bindingLayout = contractPackage.bindingLayout;
			package.packageKind = contractPackage.packageKind;
			package.packageFeatures = contractPackage.packageFeatures;
			package.rayTracingPayloadSizeInBytes = contractPackage.rayTracingPayloadSizeInBytes;
			package.rayTracingAttributeSizeInBytes = contractPackage.rayTracingAttributeSizeInBytes;
			package.rayTracingMaxRecursionDepth = contractPackage.rayTracingMaxRecursionDepth;
			packages.push_back(std::move(package));
			packageIt = packageIndices.emplace(packageId, packages.size() - 1).first;
		}

		ShaderCookPackageDesc& package = packages[packageIt->second];
		if (package.bindingLayoutId != contractPackage.bindingLayoutId)
		{
			outErrorMessage = std::format(
			    "Typed shader package '{}' mixes binding layouts '{}' and '{}'",
			    packageId,
			    package.bindingLayoutId,
			    contractPackage.bindingLayoutId);
			return {};
		}
		if (package.packageKind != contractPackage.packageKind)
		{
			outErrorMessage = std::format(
			    "Typed shader package '{}' mixes package kinds {} and {}",
			    packageId,
			    static_cast<std::uint32_t>(package.packageKind),
			    static_cast<std::uint32_t>(contractPackage.packageKind));
			return {};
		}
		for (const ShaderContractStage& stage : contractPackage.stages)
		{
			const std::uint32_t stageIndex = static_cast<std::uint32_t>(package.stages.size());
			package.stages.push_back(ShaderCookStageDesc{
			    .stage = stage.stage,
			    .sourcePath = stage.sourcePath,
			    .entryPoint = stage.entryPoint,
			    .packageKind = stage.packageKind,
			    .packageFeatures = stage.packageFeatures,
			    .rayTracingExportKind = stage.rayTracingExportKind,
			    .rayTracingExportName = stage.rayTracingExportName});

			if (stage.packageKind == CookedShaderPackageKind::RayTracingLibrary)
			{
				package.rayTracingExports.push_back(ShaderCookRayTracingExportDesc{
				    .exportLookupName = stage.rayTracingExportName,
				    .kind = stage.rayTracingExportKind,
				    .exportName = stage.rayTracingExportName,
				    .entryPoint = stage.entryPoint,
				    .stageIndex = stageIndex});
			}
		}

		for (const ShaderContractRayTracingHitGroup& hitGroup : contractPackage.rayTracingHitGroups)
		{
			if (package.packageKind != CookedShaderPackageKind::RayTracingLibrary)
			{
				outErrorMessage = std::format("Ray tracing hit group '{}' targets non-RT shader package '{}'", hitGroup.name, packageId);
				return {};
			}
			package.rayTracingHitGroups.push_back(ShaderCookRayTracingHitGroupDesc{
			    .name = hitGroup.name,
			    .closestHitExportName = hitGroup.closestHitExportName,
			    .anyHitExportName = hitGroup.anyHitExportName,
			    .intersectionExportName = hitGroup.intersectionExportName});
		}
	}

	outErrorMessage.clear();
	return packages;
}
