#include "PCH.h"

#include "Cooking/ShaderPackageCooker.h"

#include "Compiler/ShaderCompileOptionsBuilder.h"
#include "Cooking/Cache/IncludeClosureHasher.h"
#include "Cooking/Cache/LocalDiskShaderArtifactStore.h"
#include "Cooking/Cache/ShaderCacheKey.h"
#include "Cooking/Cache/ShaderCompileOptionsHasher.h"
#include "Cooking/CookedPackageWriter.h"
#include "Cooking/CookedRegistryWriter.h"
#include "Cooking/CookedStageBuild.h"
#include "Cooking/Execution/SerialCookExecutor.h"
#include "Cooking/Graph/DependencyGraph.h"
#include "Cooking/StageCompiler.h"
#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Paths/PathUtils.h"
#include "Manifest/ShaderCookManifest.h"
#include "RHI/Public/Shaders/CookedShaderPackage.h"
#include "RHI/Public/Shaders/ShaderPackageLayoutCatalog.h"

#include <format>

std::filesystem::path ShaderPackageCooker::ResolveCacheDirectory(const ShaderPackageCookSettings& settings)
{
	if (!settings.cacheDirectory.empty())
	{
		return settings.cacheDirectory;
	}

	return Engine::Paths::Normalize(Filesystem::GetExecutableDirectory().parent_path() / "Cache" / "Shaders");
}

ShaderPackageCookResult ShaderPackageCooker::CookAll(const ShaderPackageCookSettings& settings) const
{
	struct PackageCookContext final
	{
		PassParameterLayout bindingLayout;
		std::vector<CookedStageBuild> compiledStages;
	};

	ShaderPackageCookResult result;
	result.cacheDirectory = ResolveCacheDirectory(settings);

	ShaderCookManifest manifest;
	if (!manifest.LoadMerged(result.errorMessage))
	{
		return result;
	}

	std::vector<PackageCookContext> packageContexts(manifest.GetPackages().size());
	DependencyGraph graph;
	for (std::size_t packageIndex = 0; packageIndex < manifest.GetPackages().size(); ++packageIndex)
	{
		const ShaderCookPackageDesc& package = manifest.GetPackages()[packageIndex];
		PackageCookContext& packageContext = packageContexts[packageIndex];

		if (!ShaderPackageLayouts::TryBuild(package.bindingLayoutId, packageContext.bindingLayout, result.errorMessage))
		{
			result.errorMessage =
			    "Failed to build binding layout for shader package '" + package.packageId + "' - " + result.errorMessage;
			return result;
		}

		packageContext.compiledStages.reserve(package.stages.size());
		for (std::size_t stageIndex = 0; stageIndex < package.stages.size(); ++stageIndex)
		{
			const ShaderCookStageDesc& stage = package.stages[stageIndex];
			const ShaderCompileOptions compileOptions = ShaderCompileOptionsBuilder::Build(stage);
			const IncludeClosureHashResult includeHashResult = IncludeClosureHasher::Compute(compileOptions);
			if (!includeHashResult.Succeeded())
			{
				result.errorMessage = std::format(
				    "Failed to compute include closure for shader package '{}' variant '{}' stage '{}' - {}",
				    package.packageId,
				    package.variantId,
				    GetShaderStagePrefix(stage.stage),
				    includeHashResult.errorMessage);
				return result;
			}

			const std::uint64_t optionsHash = ShaderCompileOptionsHasher::Compute(compileOptions);
			graph.AddNode(CookNode{
			    .packageIndex = packageIndex,
			    .stageIndex = stageIndex,
			    .package = &package,
			    .stage = &stage,
			    .compileOptions = compileOptions,
			    .sourceHash = includeHashResult.sourceHash,
			    .includeClosureHash = includeHashResult.includeClosureHash,
			    .optionsHash = optionsHash,
			    .cacheKey = ShaderCacheKey::Compute(
			        package,
			        stage,
			        compileOptions,
			        includeHashResult.sourceHash,
			        includeHashResult.includeClosureHash,
			        optionsHash)});
		}
	}

	LocalDiskShaderArtifactStore artifactStore(result.cacheDirectory);
	SerialCookExecutor executor;
	if (!executor.Execute(
	        graph,
	        [&](const CookNode& node, std::string& outErrorMessage) -> bool
	        {
		        CookedStageBuild compiledStage;
		        if (settings.useCache)
		        {
			        std::string cacheLookupError;
			        if (artifactStore.TryGet(node.cacheKey, compiledStage, cacheLookupError))
			        {
				        ++result.cacheHitCount;
				        packageContexts[node.packageIndex].compiledStages.push_back(std::move(compiledStage));
				        outErrorMessage.clear();
				        return true;
			        }

			        if (!cacheLookupError.empty())
			        {
				        outErrorMessage = cacheLookupError;
				        return false;
			        }
		        }

		        ++result.cacheMissCount;
		        ++result.backendInvocationCount;
		        if (!StageCompiler::Compile(*node.stage, node.compileOptions, compiledStage, outErrorMessage))
		        {
			        outErrorMessage = std::format(
			            "Failed to compile shader package '{}' variant '{}' stage '{}' - {}",
			            node.package->packageId,
			            node.package->variantId,
			            GetShaderStagePrefix(node.stage->stage),
			            outErrorMessage);
			        return false;
		        }

		        if (settings.useCache)
		        {
			        std::string cachePutError;
			        if (!artifactStore.Put(node.cacheKey, compiledStage, cachePutError))
			        {
				        outErrorMessage = cachePutError;
				        return false;
			        }
		        }

		        packageContexts[node.packageIndex].compiledStages.push_back(std::move(compiledStage));
		        outErrorMessage.clear();
		        return true;
	        },
	        result.errorMessage))
	{
		result.packages.clear();
		return result;
	}

	result.packages.reserve(manifest.GetPackages().size());
	for (std::size_t packageIndex = 0; packageIndex < manifest.GetPackages().size(); ++packageIndex)
	{
		const ShaderCookPackageDesc& package = manifest.GetPackages()[packageIndex];
		const PackageCookContext& packageContext = packageContexts[packageIndex];

		CookedShaderPackageOutput packageOutput;
		if (!CookedPackageWriter::Write(
		        package,
		        packageContext.bindingLayout,
		        packageContext.compiledStages,
		        packageOutput,
		        result.errorMessage))
		{
			result.errorMessage =
			    "Failed to emit cooked shader package '" + package.packageId + "' - " + result.errorMessage;
			result.packages.clear();
			return result;
		}

		result.packages.push_back(std::move(packageOutput));
	}

	if (!CookedRegistryWriter::Write(result.packages, result.registryPath, result.errorMessage))
	{
		result.packages.clear();
		return result;
	}

	return result;
}

