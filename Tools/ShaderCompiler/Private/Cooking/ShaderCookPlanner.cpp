#include "PCH.h"

#include "Cooking/ShaderCookPlanner.h"

#include "Core/Public/Assets/AssetTypes.h"
#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Paths/PathUtils.h"
#include "Shaders/Authoring/GlobalShader.h"

#include <format>
#include <unordered_map>

ShaderCompileOptions ShaderCookPlanner::BuildCompileOptions(const ShaderCookStageDesc& stage)
{
	ShaderCompileOptions options{};
	options.SourcePath = Filesystem::ResolveAssetPathValidated(stage.sourcePath, AssetType::Shader);
	options.EntryPoint = stage.entryPoint;
	options.Stage = stage.stage;

	const std::filesystem::path& projectShaderRoot = Filesystem::GetShaderPath(PathRoot::Project);
	const std::filesystem::path& engineShaderRoot = Filesystem::GetShaderPath(PathRoot::Engine);

	if (!projectShaderRoot.empty())
	{
		options.IncludeDir = projectShaderRoot;
		if (!engineShaderRoot.empty() &&
		    Engine::Paths::MakePathKey(engineShaderRoot) != Engine::Paths::MakePathKey(projectShaderRoot))
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
		if (shader.Stage != options.Stage || shader.EntryPoint != options.EntryPoint || !sourceMatches)
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
	    .entryPoint = "VSMain"});
	package.stages.push_back(ShaderCookStageDesc{
	    .stage = ShaderStage::Pixel,
	    .sourcePath = settings.singleShaderPath,
	    .entryPoint = "PSMain"});

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
	for (const ShaderRegistrationDesc& shader : GlobalShaderRegistry::GetRegistrations())
	{
		const std::string packageId(shader.PackageName.empty() ? shader.ShaderName : shader.PackageName);
		if (!requested.empty() && packageId != requested && std::string(shader.ShaderName) != requested)
		{
			continue;
		}

		const std::string bindingLayoutId(shader.BindingLayoutId.empty() ? "Empty" : shader.BindingLayoutId);
		auto packageIt = packageIndices.find(packageId);
		if (packageIt == packageIndices.end())
		{
			PassParameterLayout bindingLayout = shader.BuildPackageBindingLayout != nullptr ? shader.BuildPackageBindingLayout() : PassParameterLayout(bindingLayoutId.c_str());
			ShaderCookPackageDesc package;
			package.packageId = packageId;
			package.bindingLayoutId = bindingLayoutId;
			package.bindingLayout = std::move(bindingLayout);
			package.variantId = "Default";
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

		const PassParameterLayout shaderBindingLayout = shader.BuildPackageBindingLayout != nullptr ? shader.BuildPackageBindingLayout() : PassParameterLayout(bindingLayoutId.c_str());
		if (BuildPassParameterLayoutHash(package.bindingLayout) != BuildPassParameterLayoutHash(shaderBindingLayout))
		{
			outErrorMessage = std::format("Typed shader package '{}' has inconsistent binding layout declarations", packageId);
			return {};
		}

		package.stages.push_back(ShaderCookStageDesc{
		    .stage = shader.Stage,
		    .sourcePath = std::filesystem::path(std::string(shader.SourcePath)),
		    .entryPoint = std::string(shader.EntryPoint)});
	}

	if (!requested.empty() && packages.empty())
	{
		outErrorMessage = "Unknown typed shader package or shader '" + requested + "'";
		return {};
	}

	outErrorMessage.clear();
	return packages;
}