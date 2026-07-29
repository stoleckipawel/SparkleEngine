#include "PCH.h"

#include "Contracts/ShaderContractCatalogBuilder.h"

#include "Shaders/Authoring/GlobalShader.h"
#include "Shaders/ShaderPackageLayoutBuilder.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Diagnostics/Verify.h"

#include <algorithm>
#include <format>
#include <unordered_map>

static const auto g_shaderContractCatalogBuilderLogger = Logging::GetOrCreateLogger("ShaderCompiler.ContractCatalogBuilder");

class ShaderContractCatalogAssembly final
{
  public:
	static ShaderContractStage BuildStageContract(const ShaderRegistrationDesc& shader)
	{
		ShaderContractStage stage;
		stage.shaderName = std::string(shader.ShaderName);
		stage.packageId = GetShaderRegistrationPackageId(shader);
		stage.bindingLayoutId = GetShaderRegistrationBindingLayoutId(shader);
		stage.sourcePath = std::filesystem::path(std::string(shader.SourcePath));
		stage.entryPoint = std::string(shader.EntryPoint);
		stage.stage = shader.Stage;
		stage.packageKind = shader.PackageKind;
		stage.packageFeatures = shader.PackageFeatures;
		stage.rayTracingExportKind = shader.RayTracingExportKind;
		stage.rayTracingExportName = shader.RayTracingExportName.empty() ? stage.entryPoint : std::string(shader.RayTracingExportName);
		stage.rayTracingPayloadSizeInBytes = shader.RayTracingPayloadSizeInBytes;
		stage.rayTracingAttributeSizeInBytes = shader.RayTracingAttributeSizeInBytes;
		stage.rayTracingMaxRecursionDepth = shader.RayTracingMaxRecursionDepth;
		if (shader.BuildParameterStructDescriptor != nullptr)
		{
			stage.parameterStruct = shader.BuildParameterStructDescriptor();
			stage.hasParameterStruct = true;
		}
		return stage;
	}

	static bool MatchesSelection(
	    const ShaderContractStage& stage,
	    ShaderContractSelectionKind selectionKind,
	    std::string_view requestedId) noexcept
	{
		switch (selectionKind)
		{
			case ShaderContractSelectionKind::All:
				return true;
			case ShaderContractSelectionKind::PackageId:
				return stage.packageId == requestedId;
			case ShaderContractSelectionKind::ShaderId:
				return stage.shaderName == requestedId;
			case ShaderContractSelectionKind::RegisteredId:
				return stage.shaderName == requestedId || stage.packageId == requestedId;
		}
		Diagnostics::Fatal(g_shaderContractCatalogBuilderLogger, __FILE__, __LINE__, "Unknown shader contract selection kind.");
	}

	static bool StageLess(const ShaderContractStage& lhs, const ShaderContractStage& rhs)
	{
		if (lhs.packageId != rhs.packageId)
		{
			return lhs.packageId < rhs.packageId;
		}
		if (lhs.stage != rhs.stage)
		{
			return static_cast<std::uint32_t>(lhs.stage) < static_cast<std::uint32_t>(rhs.stage);
		}
		if (lhs.sourcePath.generic_string() != rhs.sourcePath.generic_string())
		{
			return lhs.sourcePath.generic_string() < rhs.sourcePath.generic_string();
		}
		if (lhs.entryPoint != rhs.entryPoint)
		{
			return lhs.entryPoint < rhs.entryPoint;
		}
		return lhs.shaderName < rhs.shaderName;
	}

	static bool HitGroupLess(const ShaderContractRayTracingHitGroup& lhs, const ShaderContractRayTracingHitGroup& rhs)
	{
		if (lhs.packageId != rhs.packageId)
		{
			return lhs.packageId < rhs.packageId;
		}
		return lhs.name < rhs.name;
	}
};

ShaderContractCatalog ShaderContractCatalogBuilder::Build(
    ShaderContractSelectionKind selectionKind,
    std::string_view requestedId)
{
	ShaderContractCatalog catalog;
	for (const ShaderRegistrationDesc& shader : GlobalShaderRegistry::GetRegistrations())
	{
		ShaderContractStage stage = ShaderContractCatalogAssembly::BuildStageContract(shader);
		if (ShaderContractCatalogAssembly::MatchesSelection(stage, selectionKind, requestedId))
		{
			catalog.stages.push_back(std::move(stage));
		}
	}

	std::ranges::sort(catalog.stages, ShaderContractCatalogAssembly::StageLess);

	std::unordered_map<std::string, std::size_t> packageIndices;
	for (const ShaderContractStage& stage : catalog.stages)
	{
		auto packageIt = packageIndices.find(stage.packageId);
		if (packageIt == packageIndices.end())
		{
			ShaderContractPackage package;
			package.packageId = stage.packageId;
			package.bindingLayoutId = stage.bindingLayoutId;
			package.bindingLayout =
			    ShaderPackageLayoutBuilder::Build(stage.packageId, GlobalShaderRegistry::GetRegistrations());
			package.packageKind = stage.packageKind;
			package.packageFeatures = stage.packageFeatures;
			package.rayTracingPayloadSizeInBytes = stage.rayTracingPayloadSizeInBytes;
			package.rayTracingAttributeSizeInBytes = stage.rayTracingAttributeSizeInBytes;
			package.rayTracingMaxRecursionDepth = stage.rayTracingMaxRecursionDepth;
			catalog.packages.push_back(std::move(package));
			packageIt = packageIndices.emplace(stage.packageId, catalog.packages.size() - 1u).first;
		}

		ShaderContractPackage& package = catalog.packages[packageIt->second];
		package.packageFeatures |= stage.packageFeatures;
		package.rayTracingPayloadSizeInBytes = std::max(package.rayTracingPayloadSizeInBytes, stage.rayTracingPayloadSizeInBytes);
		package.rayTracingAttributeSizeInBytes = std::max(package.rayTracingAttributeSizeInBytes, stage.rayTracingAttributeSizeInBytes);
		package.rayTracingMaxRecursionDepth = std::max(package.rayTracingMaxRecursionDepth, stage.rayTracingMaxRecursionDepth);
		package.stages.push_back(stage);
	}

	for (const RayTracingHitGroupRegistrationDesc& hitGroup : GlobalShaderRegistry::GetRayTracingHitGroups())
	{
		const std::string packageId(hitGroup.PackageName);
		const bool selected = selectionKind == ShaderContractSelectionKind::All ||
		    (selectionKind == ShaderContractSelectionKind::PackageId && packageId == requestedId);
		if (!selected)
		{
			continue;
		}

		auto packageIt = packageIndices.find(packageId);
		if (packageIt == packageIndices.end())
		{
			continue;
		}

		catalog.packages[packageIt->second].rayTracingHitGroups.push_back(
		    ShaderContractRayTracingHitGroup{
		        .packageId = packageId,
		        .name = std::string(hitGroup.HitGroupName),
		        .closestHitExportName = std::string(hitGroup.ClosestHitExportName),
		        .anyHitExportName = std::string(hitGroup.AnyHitExportName),
		        .intersectionExportName = std::string(hitGroup.IntersectionExportName)});
	}

	for (ShaderContractPackage& package : catalog.packages)
	{
		std::ranges::sort(package.rayTracingHitGroups, ShaderContractCatalogAssembly::HitGroupLess);
	}

	if (selectionKind != ShaderContractSelectionKind::All && catalog.packages.empty())
	{
		const std::string subject = selectionKind == ShaderContractSelectionKind::PackageId
		    ? "typed shader package"
		    : selectionKind == ShaderContractSelectionKind::ShaderId ? "registered shader" : "registered shader or package";
		throw Diagnostics::Error(std::format("Unknown {} '{}'.", subject, requestedId));
	}

	return catalog;
}
