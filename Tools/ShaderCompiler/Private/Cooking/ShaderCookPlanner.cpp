#include "PCH.h"

#include "Cooking/ShaderCookPlanner.h"

#include "Core/Public/Assets/AssetTypes.h"
#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Paths/DirectoryPaths.h"
#include "Core/Public/Paths/PathUtils.h"
#include "Shaders/Authoring/GlobalShader.h"
#include "Shaders/ShaderPackageLayoutBuilder.h"

#include <format>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>

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
	if (!settings.singleShaderPath.empty() && settings.singleShaderPath.has_extension())
	{
		std::string typedRequestError;
		std::vector<ShaderCookPackageDesc> typedPackages = BuildTypedShaderPackages(settings.singleShaderPath.generic_string(), typedRequestError);
		if (!typedPackages.empty())
		{
			outErrorMessage.clear();
			return typedPackages;
		}

		outErrorMessage.clear();
		return BuildSingleShaderPackage(settings);
	}

	const std::string requestedPackageId = settings.singleShaderPath.empty() ? std::string{} : settings.singleShaderPath.generic_string();
	std::vector<ShaderCookPackageDesc> packages = BuildTypedShaderPackages(requestedPackageId, outErrorMessage);
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

std::vector<ShaderCookPackageDesc> ShaderCookPlanner::BuildSingleShaderPackage(const ShaderPackageCookSettings& settings)
{
	ShaderCookPackageDesc package;
	package.packageId = settings.singleShaderPath.stem().string();
	package.bindingLayoutId = "Empty";
	package.bindingLayout = PassParameterLayout("Empty");
	package.variantId = "Default";
	package.stages.push_back(ShaderCookStageDesc{
	    .stage = ShaderStage::Vertex,
	    .sourcePath = settings.singleShaderPath,
		    .entryPoint = "VSMain",
		    .packageKind = CookedShaderPackageKind::Graphics});
	package.stages.push_back(ShaderCookStageDesc{
	    .stage = ShaderStage::Pixel,
	    .sourcePath = settings.singleShaderPath,
		    .entryPoint = "PSMain",
		    .packageKind = CookedShaderPackageKind::Graphics});

	std::vector<ShaderCookPackageDesc> packages;
	packages.push_back(std::move(package));
	return packages;
}

std::vector<ShaderCookPackageDesc> ShaderCookPlanner::BuildTypedShaderPackages(
	std::string_view requestedPackageId,
	std::string& outErrorMessage)
{
	std::vector<ShaderCookPackageDesc> packages;
	std::unordered_map<std::string, std::size_t> packageIndices;
	const std::string requested(requestedPackageId);
	std::unordered_set<std::string> packageIdsSelectedBySource;
	if (!requested.empty())
	{
		for (const ShaderRegistrationDesc& shader : GlobalShaderRegistry::GetRegistrations())
		{
			const std::string shaderName(shader.ShaderName);
			const std::string sourcePath(shader.SourcePath);
			const bool sourceMatches = sourcePath == requested || sourcePath.ends_with("/" + requested) || requested.ends_with("/" + sourcePath);
			if (shaderName == requested || sourceMatches)
			{
				packageIdsSelectedBySource.insert(shader.PackageName.empty() ? shaderName : std::string(shader.PackageName));
			}
		}
	}

	for (const ShaderRegistrationDesc& shader : GlobalShaderRegistry::GetRegistrations())
	{
		const std::string packageId(shader.PackageName.empty() ? shader.ShaderName : shader.PackageName);
		const std::string shaderName(shader.ShaderName);
		const std::string sourcePath(shader.SourcePath);
		const bool sourceMatches = !requested.empty() &&
		    (sourcePath == requested || sourcePath.ends_with("/" + requested) || requested.ends_with("/" + sourcePath));
		const bool packageSelectedBySource = packageIdsSelectedBySource.contains(packageId);
		if (!requested.empty() && packageId != requested && shaderName != requested && !sourceMatches && !packageSelectedBySource)
		{
			continue;
		}

		const std::string bindingLayoutId(shader.BindingLayoutId.empty() ? "Empty" : shader.BindingLayoutId);
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
			package.variantId = "Default";
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
		    .rayTracingExportName = shader.RayTracingExportName.empty() ? std::string(shader.EntryPoint) : std::string(shader.RayTracingExportName)});

		if (shader.PackageKind == CookedShaderPackageKind::RayTracingLibrary)
		{
			package.rayTracingExports.push_back(ShaderCookRayTracingExportDesc{
			    .shaderName = shaderName,
			    .kind = shader.RayTracingExportKind,
			    .exportName = shader.RayTracingExportName.empty() ? std::string(shader.EntryPoint) : std::string(shader.RayTracingExportName),
			    .entryPoint = std::string(shader.EntryPoint),
			    .stageIndex = stageIndex});
		}
	}

	for (const RayTracingHitGroupRegistrationDesc& hitGroup : GlobalShaderRegistry::GetRayTracingHitGroups())
	{
		const std::string packageId(hitGroup.PackageName);
		const bool wholePackageSelected = requested.empty() || packageId == requested || packageIdsSelectedBySource.contains(packageId);
		if (!wholePackageSelected)
		{
			continue;
		}

		auto packageIt = packageIndices.find(packageId);
		if (packageIt == packageIndices.end())
		{
			continue;
		}

		ShaderCookPackageDesc& package = packages[packageIt->second];
		if (package.packageKind != CookedShaderPackageKind::RayTracingLibrary)
		{
			outErrorMessage = std::format("Ray tracing hit group '{}' targets non-RT shader package '{}'", hitGroup.HitGroupName, packageId);
			return {};
		}

		package.rayTracingHitGroups.push_back(ShaderCookRayTracingHitGroupDesc{
		    .name = std::string(hitGroup.HitGroupName),
		    .closestHitShaderName = std::string(hitGroup.ClosestHitShaderName),
		    .anyHitShaderName = std::string(hitGroup.AnyHitShaderName),
		    .intersectionShaderName = std::string(hitGroup.IntersectionShaderName)});
	}

	if (!requested.empty() && packages.empty())
	{
		outErrorMessage = "Unknown typed shader package or shader '" + requested + "'";
		return {};
	}

	outErrorMessage.clear();
	return packages;
}