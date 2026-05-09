#include "AssetCookerDispatcher.h"

#include "AssetCookerToolProcess.h"
#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Hash/HashUtils.h"
#include "Core/Public/Paths/DirectoryPaths.h"
#include "MaterialCooker.h"
#include "MeshCooker.h"
#include "SceneCooker.h"
#include "SourceSceneImporter.h"
#include "TextureCookRequestList.h"

#include <chrono>
#include <iostream>
#include <map>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include <objbase.h>

static bool AssetCookerFileExists(const std::filesystem::path& path)
{
	std::error_code errorCode;
	return std::filesystem::exists(path, errorCode);
}

static std::filesystem::path AssetCookerResolveToolPath(
    const AssetCookerProjectCookPlan& plan,
    std::string_view executableName)
{
	const std::string fileName = std::string(executableName) + ".exe";
	const std::filesystem::path configuredPath = plan.repositoryRoot / "build" / "bin" / plan.configuration / fileName;
	if (AssetCookerFileExists(configuredPath))
	{
		return configuredPath;
	}

	return plan.repositoryRoot / "build" / "bin" / fileName;
}

static bool AssetCookerPlanUsesStep(const AssetCookerProjectCookPlan& plan, AssetCookerPlanStep step)
{
	for (const AssetCookerPlanStep planStep : plan.steps)
	{
		if (planStep == step)
		{
			return true;
		}
	}
	return false;
}

static std::filesystem::path AssetCookerMakeTempPath(
    const AssetCookerProjectCookPlan& plan,
    std::string_view stem,
    std::string_view extension)
{
	const auto timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
	return plan.repositoryRoot / "build" / "Cook" / "Temp" /
	       (std::string(stem) + "-" + plan.projectName + "-" + std::to_string(timestamp) + std::string(extension));
}

static void AssetCookerRemoveTempFile(const std::filesystem::path& path)
{
	std::error_code removeError;
	std::filesystem::remove(path, removeError);
}

static bool AssetCookerTextureCookRequestsMatch(const TextureCookRequest& lhs, const TextureCookRequest& rhs) noexcept
{
	return lhs.assetId == rhs.assetId && lhs.sourcePath == rhs.sourcePath && lhs.outputPath == rhs.outputPath &&
	       lhs.colorSpace == rhs.colorSpace && lhs.mipPolicy == rhs.mipPolicy && lhs.mipFilter == rhs.mipFilter &&
	       lhs.colorProcessingPolicy == rhs.colorProcessingPolicy && lhs.textureGroup == rhs.textureGroup &&
	       lhs.dimension == rhs.dimension && lhs.channelMask == rhs.channelMask;
}

static bool AssetCookerAddUniqueTextureCookRequest(
    const TextureCookRequest& request,
    std::map<TextureAssetId, TextureCookRequest>& requestsById,
    std::vector<TextureCookRequest>& outRequests,
    std::string& outErrorMessage)
{
	const auto existingRequest = requestsById.find(request.assetId);
	if (existingRequest == requestsById.end())
	{
		requestsById.emplace(request.assetId, request);
		outRequests.push_back(request);
		outErrorMessage.clear();
		return true;
	}

	if (!AssetCookerTextureCookRequestsMatch(existingRequest->second, request))
	{
		outErrorMessage = "Texture request conflict for asset id " + std::to_string(request.assetId) + ".";
		return false;
	}

	outErrorMessage.clear();
	return true;
}

static bool AssetCookerAppendDefaultTextureRequest(
    std::string_view sourceRelativePath,
    std::string_view outputRelativePath,
    TextureColorSpace colorSpace,
    TextureMipFilter mipFilter,
    TextureColorProcessingPolicy colorProcessingPolicy,
    TextureGroup textureGroup,
    TextureDimension dimension,
    std::map<TextureAssetId, TextureCookRequest>& requestsById,
    std::vector<TextureCookRequest>& outRequests,
    std::string& outErrorMessage)
{
	const std::filesystem::path sourcePath = (Paths::EngineRoot() / std::filesystem::path(std::string(sourceRelativePath))).lexically_normal();
	std::error_code existsError;
	if (!std::filesystem::exists(sourcePath, existsError))
	{
		outErrorMessage = "Default source texture was not found: " + sourcePath.string();
		return false;
	}

	TextureCookRequest request;
	request.assetId = Hash::Fnv1a64(std::string("engine-default-texture:") + std::string(outputRelativePath));
	request.sourcePath = sourcePath;
	request.outputPath = (Paths::CookedTextureRoot() / std::filesystem::path(std::string(outputRelativePath))).lexically_normal();
	request.colorSpace = colorSpace;
	request.mipPolicy = TextureMipPolicy::Generate;
	request.mipFilter = mipFilter;
	request.colorProcessingPolicy = colorProcessingPolicy;
	request.textureGroup = textureGroup;
	request.dimension = dimension;
	request.channelMask = TextureChannelMask::Rgba;

	if (!AssetCookerAddUniqueTextureCookRequest(request, requestsById, outRequests, outErrorMessage))
	{
		return false;
	}

	std::cout << "[LOG] Added default texture request: source='" << request.sourcePath.string() << "' output='"
	          << request.outputPath.string() << "'\n";
	outErrorMessage.clear();
	return true;
}

static bool AssetCookerAppendDefaultTextureRequests(
    std::map<TextureAssetId, TextureCookRequest>& requestsById,
    std::vector<TextureCookRequest>& outRequests,
    std::string& outErrorMessage)
{
	struct DefaultTextureCookDesc final
	{
		std::string_view sourceRelativePath;
		std::string_view outputRelativePath;
		TextureColorSpace colorSpace;
		TextureMipFilter mipFilter;
		TextureColorProcessingPolicy colorProcessingPolicy;
		TextureGroup textureGroup;
		TextureDimension dimension;
	};

	constexpr DefaultTextureCookDesc defaultTextures[] = {
	    {"Assets/Textures/Defaults/default_checkerboard.png",
	     "Defaults/default_checkerboard.stex",
	     TextureColorSpace::Srgb,
	     TextureMipFilter::Kaiser,
	     TextureColorProcessingPolicy::SrgbLinearize,
	     TextureGroup::Diffuse,
	     TextureDimension::Texture2D},
	    {"Assets/Textures/Defaults/default_white.png",
	     "Defaults/default_white.stex",
	     TextureColorSpace::Srgb,
	     TextureMipFilter::Regular,
	     TextureColorProcessingPolicy::SrgbLinearize,
	     TextureGroup::Diffuse,
	     TextureDimension::Texture2D},
	    {"Assets/Textures/Defaults/default_black.png",
	     "Defaults/default_black.stex",
	     TextureColorSpace::Srgb,
	     TextureMipFilter::Regular,
	     TextureColorProcessingPolicy::SrgbLinearize,
	     TextureGroup::Diffuse,
	     TextureDimension::Texture2D},
	    {"Assets/Textures/Defaults/default_red.png",
	     "Defaults/default_red.stex",
	     TextureColorSpace::Srgb,
	     TextureMipFilter::Regular,
	     TextureColorProcessingPolicy::SrgbLinearize,
	     TextureGroup::Diffuse,
	     TextureDimension::Texture2D},
	    {"Assets/Textures/Defaults/default_green.png",
	     "Defaults/default_green.stex",
	     TextureColorSpace::Srgb,
	     TextureMipFilter::Regular,
	     TextureColorProcessingPolicy::SrgbLinearize,
	     TextureGroup::Diffuse,
	     TextureDimension::Texture2D},
	    {"Assets/Textures/Defaults/default_blue.png",
	     "Defaults/default_blue.stex",
	     TextureColorSpace::Srgb,
	     TextureMipFilter::Regular,
	     TextureColorProcessingPolicy::SrgbLinearize,
	     TextureGroup::Diffuse,
	     TextureDimension::Texture2D},
	    {"Assets/Textures/Defaults/default_normal.png",
	     "Defaults/default_normal.stex",
	     TextureColorSpace::Linear,
	     TextureMipFilter::NormalAware,
	     TextureColorProcessingPolicy::Linear,
	     TextureGroup::NormalMap,
	     TextureDimension::Texture2D},
	    {"Assets/Textures/Sky/evening_road_01_puresky_4k.exr",
	     "Defaults/default_cubemap.stex",
	     TextureColorSpace::Linear,
	     TextureMipFilter::Regular,
	     TextureColorProcessingPolicy::Linear,
	     TextureGroup::Default,
	     TextureDimension::Texture2D}};

	for (const DefaultTextureCookDesc& defaultTexture : defaultTextures)
	{
		if (!AssetCookerAppendDefaultTextureRequest(
		        defaultTexture.sourceRelativePath,
		        defaultTexture.outputRelativePath,
		        defaultTexture.colorSpace,
		        defaultTexture.mipFilter,
		        defaultTexture.colorProcessingPolicy,
		        defaultTexture.textureGroup,
		        defaultTexture.dimension,
		        requestsById,
		        outRequests,
		        outErrorMessage))
		{
			return false;
		}
	}

	outErrorMessage.clear();
	return true;
}

template <typename ImportedSceneHandler>
static bool AssetCookerRunWithImportedScene(
    const AssetCookerSceneEntry& sceneEntry,
    AssetCookerCategory category,
    AssetCookerDiagnostics& diagnostics,
    ImportedSceneHandler&& importedSceneHandler)
{
	const HRESULT coInitializeResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	if (FAILED(coInitializeResult) && coInitializeResult != RPC_E_CHANGED_MODE)
	{
		diagnostics.AddError(category, "Failed to initialize COM for source import.", sceneEntry.sourcePath);
		return false;
	}

	SourceImportResult importResult = SourceSceneImporter::Import(sceneEntry.sourcePath);
	if (!importResult.IsValid())
	{
		if (SUCCEEDED(coInitializeResult))
		{
			CoUninitialize();
		}

		diagnostics.AddError(category, "Failed to import source scene.", sceneEntry.sourcePath);
		return false;
	}

	const bool result = importedSceneHandler(importResult);
	if (SUCCEEDED(coInitializeResult))
	{
		CoUninitialize();
	}

	return result;
}

static bool AssetCookerCookImportedScene(
    const AssetCookerSceneEntry& sceneEntry,
    const SourceImportResult& importResult,
    AssetCookerDiagnostics& diagnostics)
{
	CookedSceneBuild build;
	if (!importResult.IsValid())
	{
		diagnostics.AddError(AssetCookerCategory_SceneAssets, "Scene import result is not valid.", sceneEntry.sourcePath);
		return false;
	}

	if (!SceneCooker::ResolveSceneAsset(sceneEntry.sourcePath, build.sceneAssetId, build.sceneManifestPath, build.errorMessage))
	{
		diagnostics.AddError(AssetCookerCategory_SceneAssets, build.errorMessage, sceneEntry.sourcePath);
		return false;
	}

	MeshCooker::BuildMeshAssets(importResult, build.sceneAssetId, build.meshAssets, build.meshAssetReferences);
	if (!MaterialCooker::BuildMaterialAssets(
	        importResult,
	        build.sceneAssetId,
	        build.materialAssets,
	        build.materialAssetReferences,
	        build.errorMessage))
	{
		diagnostics.AddError(AssetCookerCategory_Material, build.errorMessage, sceneEntry.sourcePath);
		return false;
	}

	if (!SceneCooker::BuildManifest(importResult, build, build.errorMessage))
	{
		diagnostics.AddError(AssetCookerCategory_SceneAssets, build.errorMessage, sceneEntry.sourcePath);
		return false;
	}

	if (!MeshCooker::WriteMeshAssets(build.meshAssets, build.errorMessage))
	{
		diagnostics.AddError(AssetCookerCategory_Mesh, build.errorMessage, sceneEntry.sourcePath);
		return false;
	}

	if (!MaterialCooker::WriteMaterialAssets(build.materialAssets, build.errorMessage))
	{
		diagnostics.AddError(AssetCookerCategory_Material, build.errorMessage, sceneEntry.sourcePath);
		return false;
	}

	if (!SceneCooker::WriteSceneManifestAndRegistry(build, build.errorMessage))
	{
		diagnostics.AddError(AssetCookerCategory_SceneAssets, build.errorMessage, sceneEntry.sourcePath);
		return false;
	}

	std::cout << "AssetCooker: imported '" << sceneEntry.sourcePath.string() << "' via "
	          << GetSourceImporterTypeName(importResult.importerType) << " with " << importResult.GetMeshCount()
	          << " meshes and " << importResult.GetMaterialCount() << " materials; emitted scene asset '" << build.sceneAssetId
	          << "' to '" << build.sceneManifestPath.string() << "'\n";
	std::cout << "AssetCooker Scene Summary:\n"
	          << "  source='" << sceneEntry.sourcePath.string() << "'\n"
	          << "  importer='" << GetSourceImporterTypeName(importResult.importerType) << "'\n"
	          << "  meshes=" << importResult.GetMeshCount() << "\n"
	          << "  materials=" << importResult.GetMaterialCount() << "\n"
	          << "  sceneManifest='" << build.sceneManifestPath.string() << "'\n";
	return true;
}

static bool AssetCookerCollectTextureRequests(
    const AssetCookerProjectCookPlan& plan,
    AssetCookerDiagnostics& diagnostics,
    const std::filesystem::path& textureRequestPath)
{
	std::map<TextureAssetId, TextureCookRequest> requestsById;
	std::vector<TextureCookRequest> requests;
	std::vector<std::string> failedScenes;
	int collectedSceneCount = 0;
	std::string errorMessage;

	for (const AssetCookerSceneEntry& sceneEntry : plan.sceneEntries)
	{
		std::cout << "\n[LOG] Collecting texture requests [" << (collectedSceneCount + 1) << "/" << plan.sceneEntries.size()
		          << "] " << sceneEntry.origin << ": " << sceneEntry.relativePath << "\n";

		std::vector<TextureCookRequest> sceneRequests;
		const bool collected = AssetCookerRunWithImportedScene(
		    sceneEntry,
		    AssetCookerCategory_Textures,
		    diagnostics,
		    [&](const SourceImportResult& importResult) -> bool
		    {
			    if (!MaterialCooker::CollectTextureCookRequests(importResult, sceneRequests, errorMessage))
			    {
				    diagnostics.AddError(AssetCookerCategory_Textures, errorMessage, sceneEntry.sourcePath);
				    return false;
			    }

			    return true;
		    });
		if (!collected)
		{
			failedScenes.push_back(sceneEntry.origin + ":" + sceneEntry.relativePath);
			++collectedSceneCount;
			continue;
		}

		for (const TextureCookRequest& request : sceneRequests)
		{
			if (!AssetCookerAddUniqueTextureCookRequest(request, requestsById, requests, errorMessage))
			{
				diagnostics.AddError(AssetCookerCategory_Textures, errorMessage, sceneEntry.sourcePath);
				return false;
			}
		}

		++collectedSceneCount;
	}

	if (!failedScenes.empty())
	{
		diagnostics.AddError(
		    AssetCookerCategory_Textures,
		    "Texture request collection failed for " + std::to_string(failedScenes.size()) + " scene(s).");
		return false;
	}

	if (!AssetCookerAppendDefaultTextureRequests(requestsById, requests, errorMessage))
	{
		diagnostics.AddError(AssetCookerCategory_Textures, errorMessage);
		return false;
	}

	if (!WriteTextureCookRequestList(textureRequestPath, requests, errorMessage))
	{
		diagnostics.AddError(AssetCookerCategory_Textures, errorMessage, textureRequestPath);
		return false;
	}

	std::cout << "\n[LOG] Collected " << requests.size() << " unique texture request(s) into " << textureRequestPath.string()
	          << "\n";
	return true;
}

static bool AssetCookerRunShaders(
    const AssetCookerProjectCookPlan& plan,
    AssetCookerDiagnostics& diagnostics,
    std::vector<AssetCookerOutputRecord>& outOutputs)
{
	const std::filesystem::path shaderCompilerPath = AssetCookerResolveToolPath(plan, "ShaderCompiler");
	int exitCode = AssetCookerToolProcess::Run(shaderCompilerPath, {L"list-shaders", L"--validate"}, plan.projectRoot);
	if (exitCode != 0)
	{
		diagnostics.AddError(AssetCookerCategory_Shaders, "Shader registration validation failed.");
		return false;
	}

	exitCode = AssetCookerToolProcess::Run(shaderCompilerPath, {L"cook"}, plan.projectRoot);
	if (exitCode != 0)
	{
		diagnostics.AddError(AssetCookerCategory_Shaders, "Shader package cooking failed.");
		return false;
	}

	AssetCookerOutputRecord packagesOutput;
	packagesOutput.category = AssetCookerCategory_Shaders;
	packagesOutput.assetId = "shader-packages";
	packagesOutput.path = (plan.cookedRoot / "Shaders" / "Packages").string();
	packagesOutput.reloadHint = "Reload shader packages and affected pipelines.";
	outOutputs.push_back(std::move(packagesOutput));

	AssetCookerOutputRecord registryOutput;
	registryOutput.category = AssetCookerCategory_Shaders;
	registryOutput.assetId = "shader-registry";
	registryOutput.path = (plan.cookedRoot / "Shaders" / "ShaderPackageRegistry.sreg").string();
	registryOutput.reloadHint = "Reload shader package registry.";
	outOutputs.push_back(std::move(registryOutput));
	return true;
}

static bool AssetCookerRunTextures(
    const AssetCookerProjectCookPlan& plan,
    AssetCookerDiagnostics& diagnostics,
    std::vector<AssetCookerOutputRecord>& outOutputs)
{
	const std::filesystem::path textureCookerPath = AssetCookerResolveToolPath(plan, "TextureCooker");
	const std::filesystem::path textureRequestPath = AssetCookerMakeTempPath(plan, "assetcooker-texture-requests", ".txt");

	std::error_code createError;
	std::filesystem::create_directories(textureRequestPath.parent_path(), createError);
	if (createError)
	{
		diagnostics.AddError(AssetCookerCategory_Textures, "Failed to create texture-request temp directory.", textureRequestPath.parent_path());
		return false;
	}

	if (!AssetCookerCollectTextureRequests(plan, diagnostics, textureRequestPath))
	{
		AssetCookerRemoveTempFile(textureRequestPath);
		return false;
	}

	const int exitCode = AssetCookerToolProcess::Run(textureCookerPath, {L"cook-request-file", textureRequestPath.wstring()}, plan.projectRoot);
	AssetCookerRemoveTempFile(textureRequestPath);
	if (exitCode != 0)
	{
		diagnostics.AddError(AssetCookerCategory_Textures, "Texture asset cooking failed.");
		return false;
	}

	AssetCookerOutputRecord output;
	output.category = AssetCookerCategory_Textures;
	output.assetId = "textures";
	output.path = (plan.cookedRoot / "Textures").string();
	output.reloadHint = "Reload changed cooked textures.";
	outOutputs.push_back(std::move(output));
	return true;
}

static bool AssetCookerRunSceneAssets(
    const AssetCookerProjectCookPlan& plan,
    AssetCookerDiagnostics& diagnostics,
    std::vector<AssetCookerOutputRecord>& outOutputs)
{
	std::vector<std::string> failedScenes;
	int cookedSceneCount = 0;
	for (const AssetCookerSceneEntry& sceneEntry : plan.sceneEntries)
	{
		std::cout << "\n[LOG] Cooking [" << (cookedSceneCount + 1) << "/" << plan.sceneEntries.size() << "] "
		          << sceneEntry.origin << ": " << sceneEntry.relativePath << "\n";

		const bool cooked = AssetCookerRunWithImportedScene(
		    sceneEntry,
		    AssetCookerCategory_SceneAssets,
		    diagnostics,
		    [&](const SourceImportResult& importResult) -> bool
		    {
			    return AssetCookerCookImportedScene(sceneEntry, importResult, diagnostics);
		    });
		if (!cooked)
		{
			failedScenes.push_back(sceneEntry.origin + ":" + sceneEntry.relativePath);
			continue;
		}

		++cookedSceneCount;
	}

	if (!failedScenes.empty())
	{
		diagnostics.AddError(
		    AssetCookerCategory_SceneAssets,
		    "Scene, mesh, and material asset cooking failed for " + std::to_string(failedScenes.size()) + " scene(s).");
		return false;
	}

	AssetCookerOutputRecord sceneOutput;
	sceneOutput.category = AssetCookerCategory_SceneAssets;
	sceneOutput.assetId = "scene-manifests";
	sceneOutput.path = (plan.cookedRoot / "SceneManifests").string();
	sceneOutput.reloadHint = "Reload changed scenes and referenced cooked assets.";
	outOutputs.push_back(std::move(sceneOutput));

	AssetCookerOutputRecord meshOutput;
	meshOutput.category = AssetCookerCategory_Mesh;
	meshOutput.assetId = "meshes";
	meshOutput.path = (plan.cookedRoot / "Meshes").string();
	meshOutput.reloadHint = "Reload changed cooked meshes.";
	outOutputs.push_back(std::move(meshOutput));

	AssetCookerOutputRecord materialOutput;
	materialOutput.category = AssetCookerCategory_Material;
	materialOutput.assetId = "materials";
	materialOutput.path = (plan.cookedRoot / "Materials").string();
	materialOutput.reloadHint = "Reload changed cooked materials.";
	outOutputs.push_back(std::move(materialOutput));
	return true;
}

bool AssetCookerDispatcher::ValidateCapabilities(
    const AssetCookerProjectCookPlan& plan,
    AssetCookerDiagnostics& diagnostics)
{
	bool result = true;
	if (AssetCookerPlanUsesStep(plan, AssetCookerPlanStep::Shaders))
	{
		const std::filesystem::path shaderCompilerPath = AssetCookerResolveToolPath(plan, "ShaderCompiler");
		if (!AssetCookerFileExists(shaderCompilerPath))
		{
			diagnostics.AddError(AssetCookerCategory_Shaders, "ShaderCompiler executable was not found.", shaderCompilerPath);
			result = false;
		}
	}

	if (AssetCookerPlanUsesStep(plan, AssetCookerPlanStep::Textures))
	{
		const std::filesystem::path textureCookerPath = AssetCookerResolveToolPath(plan, "TextureCooker");
		if (!AssetCookerFileExists(textureCookerPath))
		{
			diagnostics.AddError(AssetCookerCategory_Textures, "TextureCooker executable was not found.", textureCookerPath);
			result = false;
		}
	}

	return result;
}

bool AssetCookerDispatcher::DispatchPlan(
    const AssetCookerProjectCookPlan& plan,
    AssetCookerDiagnostics& diagnostics,
    std::vector<AssetCookerOutputRecord>& outOutputs)
{
	Filesystem::ConfigureProjectRoot(plan.projectRoot);

	std::cout << "AssetCooker Plan:\n"
	          << "  project=" << plan.projectName << "\n"
	          << "  configuration=" << plan.configuration << "\n"
	          << "  engineScenes=" << plan.engineSceneCount << "\n"
	          << "  projectScenes=" << plan.projectSceneCount << "\n"
	          << "  overrides=" << plan.overriddenEngineSceneCount << "\n"
	          << "  finalScenes=" << plan.sceneEntries.size() << "\n";

	for (const AssetCookerPlanStep step : plan.steps)
	{
		if (step == AssetCookerPlanStep::Shaders && !AssetCookerRunShaders(plan, diagnostics, outOutputs))
		{
			return false;
		}

		if (step == AssetCookerPlanStep::Textures && !AssetCookerRunTextures(plan, diagnostics, outOutputs))
		{
			return false;
		}

		if (step == AssetCookerPlanStep::SceneAssets && !AssetCookerRunSceneAssets(plan, diagnostics, outOutputs))
		{
			return false;
		}
	}

	return true;
}
