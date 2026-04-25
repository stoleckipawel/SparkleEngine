#include "PCH.h"

#include "Cooking/ShaderPackageCooker.h"

#include "Backend/ShaderBackendFactory.h"
#include "Cooking/Cache/IncludeClosureHasher.h"
#include "Cooking/Cache/LocalDiskShaderArtifactStore.h"
#include "Cooking/Cache/ShaderCacheKey.h"
#include "Cooking/Cache/ShaderCompileOptionsHasher.h"
#include "Cooking/CookedPackageWriter.h"
#include "Cooking/CookedRegistryWriter.h"
#include "Cooking/CookedStageBuild.h"
#include "Cooking/ShaderCookPlanner.h"
#include "Cooking/ShaderDebugArtifactWriter.h"
#include "Cooking/ShaderRecookSignal.h"
#include "Cooking/Execution/SerialCookExecutor.h"
#include "Cooking/Graph/DependencyGraph.h"
#include "Cooking/StageCompiler.h"
#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Paths/PathUtils.h"
#include "RHI/Public/Shaders/CookedShaderPackage.h"
#include "RHI/Public/Shaders/CookedShaderPackageUtils.h"
#include "Verification/ShaderParameterStructVerifier.h"

#include <format>
#include <unordered_map>

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
		std::vector<CookedStageBuild> compiledStages;
	};

	ShaderPackageCookResult result;
	result.cacheDirectory = ResolveCacheDirectory(settings);
	const bool writeDebugArtifacts = !settings.debugArtifactDirectory.empty();
	std::unordered_map<std::string, std::unique_ptr<IShaderBackend>> backends;

	auto acquireBackend = [&](std::string_view backendName, std::string& outErrorMessage) -> IShaderBackend*
	{
		const auto existing = backends.find(std::string(backendName));
		if (existing != backends.end())
		{
			return existing->second.get();
		}

		std::unique_ptr<IShaderBackend> backend = CreateShaderBackend(backendName, outErrorMessage);
		if (!backend)
		{
			return nullptr;
		}

		const std::string resolvedName(backend->GetBackendName());
		if (!backend->GetCapabilities().SupportsTarget(settings.target))
		{
			outErrorMessage = std::string{"Shader backend '"} + resolvedName + "' does not support target '" +
				GetShaderTargetName(settings.target) + "'";
			return nullptr;
		}

		IShaderBackend* backendPtr = backend.get();
		backends.emplace(resolvedName, std::move(backend));
		outErrorMessage.clear();
		return backendPtr;
	};

	std::vector<ShaderCookPackageDesc> packages = ShaderCookPlanner::BuildPackages(settings, result.errorMessage);
	if (!result.errorMessage.empty())
	{
		return result;
	}

	std::vector<PackageCookContext> packageContexts(packages.size());
	DependencyGraph graph;
	for (std::size_t packageIndex = 0; packageIndex < packages.size(); ++packageIndex)
	{
		const ShaderCookPackageDesc& package = packages[packageIndex];
		PackageCookContext& packageContext = packageContexts[packageIndex];

		packageContext.compiledStages.reserve(package.stages.size());
		for (std::size_t stageIndex = 0; stageIndex < package.stages.size(); ++stageIndex)
		{
			const ShaderCookStageDesc& stage = package.stages[stageIndex];
			ShaderCompileOptions compileOptions = ShaderCookPlanner::BuildCompileOptions(stage);
			compileOptions.Target = settings.target;
			compileOptions.CaptureDebugArtifacts = writeDebugArtifacts;
			std::string backendSelectionError;
			const std::string backendName = ResolveShaderBackendName(
			    compileOptions.SourcePath,
			    compileOptions.Target,
			    settings.backendName,
			    backendSelectionError);
			if (backendName.empty())
			{
				result.errorMessage = std::format(
				    "Failed to select shader backend for shader package '{}' variant '{}' stage '{}' - {}",
				    package.packageId,
				    package.variantId,
				    GetShaderStagePrefix(stage.stage),
				    backendSelectionError);
				return result;
			}

			IShaderBackend* backend = acquireBackend(backendName, backendSelectionError);
			if (backend == nullptr)
			{
				result.errorMessage = std::format(
				    "Failed to construct shader backend '{}' for shader package '{}' variant '{}' stage '{}' - {}",
				    backendName,
				    package.packageId,
				    package.variantId,
				    GetShaderStagePrefix(stage.stage),
				    backendSelectionError);
				return result;
			}

			const std::string resolvedBackendName(backend->GetBackendName());

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
			    .backendName = resolvedBackendName,
			    .compileOptions = compileOptions,
			    .parameterStructDescriptor = ShaderCookPlanner::FindParameterStructDescriptor(compileOptions),
			    .sourceHash = includeHashResult.sourceHash,
			    .includeClosureHash = includeHashResult.includeClosureHash,
			    .optionsHash = optionsHash,
			    .cacheKey = ShaderCacheKey::Compute(
			        package,
			        stage,
			        compileOptions,
			        includeHashResult.sourceHash,
			        includeHashResult.includeClosureHash,
			        optionsHash,
			        resolvedBackendName,
			        backend->GetBackendVersion())});
		}
	}

	LocalDiskShaderArtifactStore artifactStore(result.cacheDirectory);
	SerialCookExecutor executor;
	if (!executor.Execute(
	        graph,
	        [&](const CookNode& node, std::string& outErrorMessage) -> bool
	        {
		        const auto backendIt = backends.find(node.backendName);
		        if (backendIt == backends.end() || !backendIt->second)
		        {
			        outErrorMessage = "Selected shader backend '" + node.backendName + "' is unavailable";
			        return false;
		        }

		        IShaderBackend& backend = *backendIt->second;
		        CookedStageBuild compiledStage;
		        auto verifyParameterStruct = [&](ShaderDebugArtifactSet* debugArtifacts) -> bool
		        {
			        if (!node.parameterStructDescriptor.has_value())
			        {
				        return true;
			        }

			        ShaderParameterStructDescriptor descriptor = *node.parameterStructDescriptor;
			        if (settings.forceParameterStructMismatchForValidation)
			        {
				        descriptor.Fields.push_back(ShaderParameterStructFieldDescriptor{
				            .Name = "__DeliberateMissingBindingForSelfTest",
				            .Kind = CookedShaderResourceKind::ConstantBuffer,
				            .Dimension = CookedShaderResourceDimension::Buffer,
				            .ArrayCount = 1,
				            .ValueSizeInBytes = sizeof(std::uint32_t),
				            .ValueAlignmentInBytes = alignof(std::uint32_t)});
			        }

			        const ShaderParameterStructVerificationResult verificationResult =
			            ShaderParameterStructVerifier::Verify(descriptor, compiledStage.reflection);
			        if (debugArtifacts != nullptr)
			        {
				        debugArtifacts->ParameterMatchReportJson = verificationResult.BuildJsonReport();
			        }
			        if (!verificationResult.succeeded)
			        {
				        outErrorMessage = std::format(
				            "SC2001 shader package '{}' variant '{}' stage '{}' parameter-struct verification failed: {}",
				            node.package->packageId,
				            node.package->variantId,
				            GetShaderStagePrefix(node.stage->stage),
				            verificationResult.diagnostics.empty() ? "unknown mismatch" : verificationResult.diagnostics.front());
				        return false;
			        }

			        return true;
		        };
		        if (settings.useCache && !writeDebugArtifacts)
		        {
			        std::string cacheLookupError;
			        if (artifactStore.TryGet(node.cacheKey, compiledStage, cacheLookupError))
			        {
				        if (!verifyParameterStruct(nullptr))
				        {
					        return false;
				        }

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
		        ShaderDebugArtifactSet debugArtifacts;
		        if (!StageCompiler::Compile(
			        backend,
			        *node.stage,
			        node.compileOptions,
			        compiledStage,
			        writeDebugArtifacts ? &debugArtifacts : nullptr,
			        outErrorMessage))
		        {
			        outErrorMessage = std::format(
			            "Failed to compile shader package '{}' variant '{}' stage '{}' - {}",
			            node.package->packageId,
			            node.package->variantId,
			            GetShaderStagePrefix(node.stage->stage),
			            outErrorMessage);
			        return false;
		        }

		        if (!verifyParameterStruct(&debugArtifacts))
		        {
			        return false;
		        }

		        if (writeDebugArtifacts)
		        {
			        if (!ShaderDebugArtifactWriter::Write(
				        settings.debugArtifactDirectory,
				        *node.package,
				        *node.stage,
				        node.compileOptions,
				        compiledStage,
				        debugArtifacts,
				        outErrorMessage))
			        {
				        outErrorMessage = std::format(
				            "Failed to write debug artifacts for shader package '{}' variant '{}' stage '{}' - {}",
				            node.package->packageId,
				            node.package->variantId,
				            GetShaderStagePrefix(node.stage->stage),
				            outErrorMessage);
				        return false;
			        }
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

	result.packages.reserve(packages.size());
	for (std::size_t packageIndex = 0; packageIndex < packages.size(); ++packageIndex)
	{
		const ShaderCookPackageDesc& package = packages[packageIndex];
		const PackageCookContext& packageContext = packageContexts[packageIndex];

		CookedShaderPackageOutput packageOutput;
		if (!CookedPackageWriter::Write(
		        package,
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

	ShaderRecookSignalResult signalResult;
	if (!ShaderRecookSignal::Write(result.cacheDirectory, result.registryPath, signalResult, result.errorMessage))
	{
		result.packages.clear();
		return result;
	}
	result.recookSignalPath = signalResult.signalPath;
	result.recookSignalRegistryHash = signalResult.registryHash;

	return result;
}

