#include "PCH.h"

#include "Cooking/ShaderCookPlanner.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/FileSystemUtils.h"
#include "Compiler/ShaderSourceMountTable.h"
#include "Contracts/ShaderContractCatalogBuilder.h"
#include "Cooking/Dependencies/ShaderDependencyManifest.h"

#include <algorithm>
#include <format>
#include <unordered_map>

std::vector<ShaderCookPackageDesc> ShaderCookPlanner::BuildPackages(
    const ShaderPackageCookSettings& settings,
    const ShaderDependencyManifest& dependencyManifest)
{
	const std::uint32_t explicitSelectionCount = static_cast<std::uint32_t>(!settings.packageId.empty())
	    + static_cast<std::uint32_t>(!settings.shaderId.empty()) + static_cast<std::uint32_t>(!settings.changedVirtualPaths.empty());
	if (explicitSelectionCount > 1)
	{
		throw Diagnostics::Error("Use one shader cook selection: package id, shader id, or changed virtual paths.");
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
	else if (!settings.changedVirtualPaths.empty())
	{
		selectionKind = CookSelectionKind::Changed;
	}

	std::unordered_set<ShaderTypeId> affectedShaderTypes;
	if (selectionKind == CookSelectionKind::Changed)
	{
		const ShaderSourceMountTable sourceMounts(
		    Filesystem::GetShaderPath(PathRoot::Engine),
		    Filesystem::GetShaderPath(PathRoot::Project));
		std::vector<std::string> canonicalChangedPaths;
		canonicalChangedPaths.reserve(settings.changedVirtualPaths.size());
		for (const std::string& changedPath : settings.changedVirtualPaths)
		{
			canonicalChangedPaths.push_back(sourceMounts.CanonicalizeVirtualPath(changedPath));
		}
		std::ranges::sort(canonicalChangedPaths);
		canonicalChangedPaths.erase(std::unique(canonicalChangedPaths.begin(), canonicalChangedPaths.end()), canonicalChangedPaths.end());
		affectedShaderTypes = dependencyManifest.SelectAffectedShaderTypes(canonicalChangedPaths);
	}

	std::vector<ShaderCookPackageDesc> packages = BuildTypedShaderPackages(selectionKind, requestedId, affectedShaderTypes);

	if (packages.empty() && selectionKind != CookSelectionKind::Changed)
	{
		throw Diagnostics::Error("No typed shader registrations were found.");
	}

	return packages;
}

std::vector<ShaderCookPackageDesc> ShaderCookPlanner::BuildTypedShaderPackages(
    CookSelectionKind selectionKind,
    std::string_view requestedId,
    const std::unordered_set<ShaderTypeId>& affectedShaderTypes)
{
	std::vector<ShaderCookPackageDesc> packages;
	std::unordered_map<std::string, std::size_t> packageIndices;
	const std::string requested(requestedId);
	const ShaderContractSelectionKind contractSelectionKind = selectionKind == CookSelectionKind::PackageId
	    ? ShaderContractSelectionKind::PackageId
	    : selectionKind == CookSelectionKind::ShaderId ? ShaderContractSelectionKind::ShaderId
	                                                   : ShaderContractSelectionKind::All;
	const ShaderContractCatalog catalog = ShaderContractCatalogBuilder::Build(contractSelectionKind, requested);

	for (const ShaderContractPackage& contractPackage : catalog.packages)
	{
		const bool packageAffected = selectionKind != CookSelectionKind::Changed
		    || std::ranges::any_of(
		        contractPackage.stages,
		        [&affectedShaderTypes](const ShaderContractStage& stage) { return affectedShaderTypes.contains(stage.shaderTypeId); });
		if (!packageAffected)
		{
			continue;
		}

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
			throw Diagnostics::Error(
			    std::format(
			        "Typed shader package '{}' mixes binding layouts '{}' and '{}'",
			        packageId,
			        package.bindingLayoutId,
			        contractPackage.bindingLayoutId));
		}
		if (package.packageKind != contractPackage.packageKind)
		{
			throw Diagnostics::Error(
			    std::format(
			        "Typed shader package '{}' mixes package kinds {} and {}",
			        packageId,
			        static_cast<std::uint32_t>(package.packageKind),
			        static_cast<std::uint32_t>(contractPackage.packageKind)));
		}
		for (const ShaderContractStage& stage : contractPackage.stages)
		{
			if (selectionKind == CookSelectionKind::Changed && !affectedShaderTypes.contains(stage.shaderTypeId))
			{
				continue;
			}
			const std::uint32_t stageIndex = static_cast<std::uint32_t>(package.stages.size());
			package.stages.push_back(
			    ShaderCookStageDesc{
			        .shaderTypeId = stage.shaderTypeId,
			        .shaderTypeName = stage.shaderName,
			        .stage = stage.stage,
			        .sourcePath = stage.sourcePath,
			        .entryPoint = stage.entryPoint,
			        .packageKind = stage.packageKind,
			        .packageFeatures = stage.packageFeatures,
			        .rayTracingExportKind = stage.rayTracingExportKind,
			        .rayTracingExportName = stage.rayTracingExportName,
			        .parameterStructDescriptor = stage.hasParameterStruct
			            ? std::optional<ShaderParameterStructDescriptor>{stage.parameterStruct}
			            : std::optional<ShaderParameterStructDescriptor>{ShaderParameterStructDescriptor{}}});

			if (stage.packageKind == CookedShaderPackageKind::RayTracingLibrary)
			{
				package.rayTracingExports.push_back(
				    ShaderCookRayTracingExportDesc{
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
				throw Diagnostics::Error(
				    std::format("Ray tracing hit group '{}' targets non-RT shader package '{}'.", hitGroup.name, packageId));
			}
			package.rayTracingHitGroups.push_back(
			    ShaderCookRayTracingHitGroupDesc{
			        .name = hitGroup.name,
			        .closestHitExportName = hitGroup.closestHitExportName,
			        .anyHitExportName = hitGroup.anyHitExportName,
			        .intersectionExportName = hitGroup.intersectionExportName});
		}
	}

	return packages;
}
