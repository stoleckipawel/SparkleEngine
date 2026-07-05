#include "AssetCookerDispatcher.h"

#include "AssetCookerToolProcess.h"
#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Hash/HashUtils.h"
#include "CookedMeshAssetBuilder.h"
#include "CookedMeshAssetWriter.h"
#include "CookedSkeletonAssetWriter.h"
#include "CookedAnimationAssetWriter.h"
#include "MaterialCooker.h"
#include "SceneCooker.h"
#include "SourceSceneImporter.h"
#include "ToolConsole.h"
#include "TextureCookRequestList.h"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include <objbase.h>

static const char* AssetCookerPlanStepName(AssetCookerPlanStep step) noexcept
{
	switch (step)
	{
	case AssetCookerPlanStep::Shaders:
		return "shaders";
	case AssetCookerPlanStep::Textures:
		return "textures";
	case AssetCookerPlanStep::SceneAssets:
		return "scene-assets";
	default:
		return "unknown";
	}
}

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
	const std::filesystem::path artifactPath =
	    plan.repositoryRoot / "artifacts" / "dev" / "tools" / std::string(executableName) / plan.toolConfiguration / fileName;
	if (AssetCookerFileExists(artifactPath))
	{
		return artifactPath;
	}
	return plan.repositoryRoot / "build" / "bin" / plan.toolConfiguration / fileName;
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
	return plan.repositoryRoot / "artifacts" / "dev" / "tools" / "AssetCooker" / "Temp" /
	       (std::string(stem) + "-" + plan.projectName + "-" + std::to_string(timestamp) + std::string(extension));
}

static void AssetCookerRemoveTempFile(const std::filesystem::path& path)
{
	std::error_code removeError;
	std::filesystem::remove(path, removeError);
}

static bool AssetCookerAppendDefaultTextureRequest(
    std::string_view sourceRelativePath,
    std::string_view outputRelativePath,
    TextureColorSpace colorSpace,
    TextureMipFilter mipFilter,
    TextureColorProcessingPolicy colorProcessingPolicy,
    TextureGroup textureGroup,
    TextureDimension dimension,
	TextureCookRequestSet& requestSet,
    std::string& outErrorMessage)
{
	const std::filesystem::path sourcePath = (Filesystem::GetEnginePath() / std::filesystem::path(std::string(sourceRelativePath))).lexically_normal();
	std::error_code existsError;
	if (!std::filesystem::exists(sourcePath, existsError))
	{
		outErrorMessage = "Default source texture was not found: " + sourcePath.string();
		return false;
	}

	TextureCookRequest request;
	request.assetId = Hash::Fnv1a64(std::string("engine-default-texture:") + std::string(outputRelativePath));
	request.sourcePath = sourcePath;
	request.outputPath = (Filesystem::GetCookedTextureRootPath() / std::filesystem::path(std::string(outputRelativePath))).lexically_normal();
	request.policy.colorSpace = colorSpace;
	request.policy.mipPolicy = TextureMipPolicy::Generate;
	request.policy.mipFilter = mipFilter;
	request.policy.colorProcessingPolicy = colorProcessingPolicy;
	request.policy.textureGroup = textureGroup;
	request.policy.dimension = dimension;
	request.policy.channelMask = TextureChannelMask::Rgba;

	if (!requestSet.Add(request, outErrorMessage))
	{
		return false;
	}

	ToolConsole::Message(
	    std::cout,
	    ToolConsoleSeverity::Info,
	    "Queued default texture",
	    {ToolConsole::QuotedField("name", ToolConsole::PathDisplayName(request.sourcePath)),
	     ToolConsole::PathField("output", request.outputPath)});
	outErrorMessage.clear();
	return true;
}

static bool AssetCookerAppendDefaultTextureRequests(TextureCookRequestSet& requestSet, std::string& outErrorMessage)
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
		        requestSet,
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

		diagnostics.AddError(
		    category,
		    "Failed to import source scene with importer '" + std::string(importResult.GetImporterId()) + "'.",
		    importResult.GetSourcePath().empty() ? sceneEntry.sourcePath : importResult.GetSourcePath());
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

	if (!SceneCooker::ResolveSceneIdentity(sceneEntry.sourcePath, build.identity, build.status.errorMessage))
	{
		diagnostics.AddError(AssetCookerCategory_SceneAssets, build.status.errorMessage, sceneEntry.sourcePath);
		return false;
	}

	build.ApplyMeshOutput(CookedMeshAssetBuilder::BuildMeshAssets(importResult, build.identity.assetId));
	MaterialCookOutput materialOutput;
	if (!MaterialCooker::BuildMaterialAssets(importResult, build.identity.assetId, materialOutput, build.status.errorMessage))
	{
		diagnostics.AddError(AssetCookerCategory_Material, build.status.errorMessage, sceneEntry.sourcePath);
		return false;
	}
	build.ApplyMaterialOutput(std::move(materialOutput));

	if (!SceneCooker::BuildManifest(importResult, build, build.status.errorMessage))
	{
		diagnostics.AddError(AssetCookerCategory_SceneAssets, build.status.errorMessage, sceneEntry.sourcePath);
		return false;
	}

	if (!CookedMeshAssetWriter::WriteMeshAssets(build.outputs.meshAssets, build.status.errorMessage))
	{
		diagnostics.AddError(AssetCookerCategory_Mesh, build.status.errorMessage, sceneEntry.sourcePath);
		return false;
	}

	if (!MaterialCooker::WriteMaterialAssets(build.outputs.materialAssets, build.status.errorMessage))
	{
		diagnostics.AddError(AssetCookerCategory_Material, build.status.errorMessage, sceneEntry.sourcePath);
		return false;
	}

	if (!CookedSkeletonAssetWriter::WriteSkeletonAssets(build.outputs.skeletonAssets, build.status.errorMessage))
	{
		diagnostics.AddError(AssetCookerCategory_SceneAssets, build.status.errorMessage, sceneEntry.sourcePath);
		return false;
	}

	if (!CookedAnimationAssetWriter::WriteAnimationAssets(build.outputs.animationAssets, build.status.errorMessage))
	{
		diagnostics.AddError(AssetCookerCategory_SceneAssets, build.status.errorMessage, sceneEntry.sourcePath);
		return false;
	}

	if (!SceneCooker::WriteSceneManifestAndRegistry(build, build.status.errorMessage))
	{
		diagnostics.AddError(AssetCookerCategory_SceneAssets, build.status.errorMessage, sceneEntry.sourcePath);
		return false;
	}

	ToolConsole::Message(
	    std::cout,
	    ToolConsoleSeverity::Info,
	    "Cooked scene",
	    {ToolConsole::QuotedField("name", sceneEntry.relativePath),
	     ToolConsole::Field("importer", std::string(importResult.GetImporterId())),
	     ToolConsole::Field("meshPrimitives", std::to_string(importResult.GetMeshPrimitiveCount())),
	     ToolConsole::Field("meshInstances", std::to_string(importResult.GetMeshInstanceCount())),
	     ToolConsole::Field("cameras", std::to_string(importResult.GetCameraCount())),
	     ToolConsole::Field("lights", std::to_string(importResult.GetLightCount())),
	     ToolConsole::Field("materials", std::to_string(importResult.GetMaterialCount())),
	     ToolConsole::Field("materialVariants", std::to_string(importResult.GetMaterialVariantCount())),
	     ToolConsole::Field("materialVariantMappings", std::to_string(importResult.GetMaterialVariantMappingCount())),
	     ToolConsole::Field("cookedMeshAssetRefs", std::to_string(build.manifest.meshAssetReferences.size())),
	     ToolConsole::Field("cookedInstances", std::to_string(build.manifest.instances.size())),
	     ToolConsole::Field("cookedInstanceGroups", std::to_string(build.manifest.instanceGroups.size())),
	     ToolConsole::Field("cookedMaterialVariants", std::to_string(build.manifest.materialVariants.size())),
	     ToolConsole::Field("cookedVariantMappings", std::to_string(build.manifest.materialVariantMappings.size())),
	     ToolConsole::Field("cookedCameras", std::to_string(build.manifest.cameras.size())),
	     ToolConsole::Field("cookedLights", std::to_string(build.manifest.lights.size())),
	     ToolConsole::PathField("manifest", build.identity.manifestPath)});
	return true;
}

static bool AssetCookerCollectTextureRequests(
    const AssetCookerProjectCookPlan& plan,
    AssetCookerDiagnostics& diagnostics,
    const std::filesystem::path& textureRequestPath)
{
	TextureCookRequestSet requestSet;
	std::vector<std::string> failedScenes;
	int collectedSceneCount = 0;
	std::string errorMessage;

	for (const AssetCookerSceneEntry& sceneEntry : plan.sceneEntries)
	{
		ToolConsole::Progress(
		    std::cout,
		    "Collecting",
		    "texture-references",
		    collectedSceneCount + 1u,
		    plan.sceneEntries.size(),
		    sceneEntry.relativePath,
		    {ToolConsole::Field("origin", sceneEntry.origin)});

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
			if (!requestSet.Add(request, errorMessage))
			{
				diagnostics.AddError(AssetCookerCategory_Textures, errorMessage, sceneEntry.sourcePath);
				return false;
			}
		}

		ToolConsole::Message(
		    std::cout,
		    ToolConsoleSeverity::Info,
		    "Queued texture references",
		    {ToolConsole::Field("textures", std::to_string(sceneRequests.size())),
		     ToolConsole::QuotedField("scene", sceneEntry.relativePath)});

		++collectedSceneCount;
	}

	if (!failedScenes.empty())
	{
		diagnostics.AddError(
		    AssetCookerCategory_Textures,
		    "Texture request collection failed for " + std::to_string(failedScenes.size()) + " scene(s).");
		return false;
	}

	if (!AssetCookerAppendDefaultTextureRequests(requestSet, errorMessage))
	{
		diagnostics.AddError(AssetCookerCategory_Textures, errorMessage);
		return false;
	}

	std::vector<TextureCookRequest> requests;
	requestSet.MoveRequestsTo(requests);

	if (!WriteTextureCookRequestList(textureRequestPath, requests, errorMessage))
	{
		diagnostics.AddError(AssetCookerCategory_Textures, errorMessage, textureRequestPath);
		return false;
	}

	ToolConsole::Message(
	    std::cout,
	    ToolConsoleSeverity::Info,
	    "Texture request plan",
	    {ToolConsole::Field("textures", std::to_string(requests.size())), ToolConsole::PathField("requestFile", textureRequestPath)});
	return true;
}

static bool AssetCookerRunShaders(
    const AssetCookerProjectCookPlan& plan,
    AssetCookerDiagnostics& diagnostics,
    std::vector<AssetCookerOutputRecord>& outOutputs)
{
	const std::filesystem::path shaderCompilerPath = AssetCookerResolveToolPath(plan, "ShaderCompiler");
	ToolConsole::Info("Cooking shaders: writing package payloads...");
	const int exitCode = AssetCookerToolProcess::Run(shaderCompilerPath, {L"cook"}, plan.projectRoot);
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
	ToolConsole::Info("Cooking textures: building request plan from scene materials...");

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

	const int exitCode = AssetCookerToolProcess::Run(
	    textureCookerPath,
	    {L"cook-request-file", textureRequestPath.wstring()},
	    plan.projectRoot);
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
		ToolConsole::Progress(
		    std::cout,
		    "Cooking",
		    "scene",
		    cookedSceneCount + 1u,
		    plan.sceneEntries.size(),
		    sceneEntry.relativePath,
		    {ToolConsole::Field("origin", sceneEntry.origin)});

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

	ToolConsole::Summary(
	    std::cout,
	    "AssetCooker cook",
	    {ToolConsole::QuotedField("project", plan.projectName),
	     ToolConsole::QuotedField("configuration", plan.configuration),
	     ToolConsole::QuotedField("toolConfiguration", plan.toolConfiguration),
	     ToolConsole::Field("steps", std::to_string(plan.steps.size())),
	     ToolConsole::Field("scenes", std::to_string(plan.sceneEntries.size()))});

	for (std::size_t stepIndex = 0; stepIndex < plan.steps.size(); ++stepIndex)
	{
		const AssetCookerPlanStep step = plan.steps[stepIndex];
		const char* stageName = AssetCookerPlanStepName(step);
		ToolConsole::Progress(std::cout, "Cooking", "stage", stepIndex + 1u, plan.steps.size(), stageName);

		bool stageSucceeded = false;
		if (step == AssetCookerPlanStep::Shaders)
		{
			stageSucceeded = AssetCookerRunShaders(plan, diagnostics, outOutputs);
		}
		else if (step == AssetCookerPlanStep::Textures)
		{
			stageSucceeded = AssetCookerRunTextures(plan, diagnostics, outOutputs);
		}
		else if (step == AssetCookerPlanStep::SceneAssets)
		{
			stageSucceeded = AssetCookerRunSceneAssets(plan, diagnostics, outOutputs);
		}

		ToolConsole::Message(
		    std::cout,
		    ToolConsoleSeverity::Info,
		    "Stage finished",
		    {ToolConsole::QuotedField("name", stageName),
		     ToolConsole::Field("status", stageSucceeded ? "completed" : "failed")});
		if (!stageSucceeded)
		{
			return false;
		}
	}

	return true;
}
