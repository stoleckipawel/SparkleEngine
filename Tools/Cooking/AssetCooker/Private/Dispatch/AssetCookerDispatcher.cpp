#include "AssetCookerDispatcher.h"

#include "AssetCookerToolProcess.h"
#include "Core/Public/Diagnostics/Logger.h"
#include "Core/Public/Diagnostics/ScopedLogEvent.h"
#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Hash/HashUtils.h"
#include "Core/Public/Json/JsonWriter.h"
#include "Core/Public/Paths/DirectoryPaths.h"
#include "CookedMeshAssetBuilder.h"
#include "CookedMeshAssetWriter.h"
#include "CookedSkeletonAssetWriter.h"
#include "CookedAnimationAssetWriter.h"
#include "MaterialCooker.h"
#include "SceneCooker.h"
#include "SourceSceneImporter.h"
#include "ToolConsole.h"
#include "TextureCookRequestList.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include <objbase.h>

struct AssetCookerStageTiming final
{
	std::string name;
	std::uint64_t elapsedMilliseconds = 0;
	bool succeeded = false;
};

static std::uint64_t AssetCookerElapsedMilliseconds(std::chrono::steady_clock::time_point startTime) noexcept
{
	return static_cast<std::uint64_t>(
	    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count());
}

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

static const char* AssetCookerCategoryName(AssetCookerCategory category) noexcept
{
	switch (category)
	{
	case AssetCookerCategory_All:
		return "all";
	case AssetCookerCategory_Shaders:
		return "shaders";
	case AssetCookerCategory_Textures:
		return "textures";
	case AssetCookerCategory_SceneAssets:
		return "scene-assets";
	case AssetCookerCategory_Texture:
		return "texture";
	case AssetCookerCategory_Shader:
		return "shader";
	case AssetCookerCategory_Mesh:
		return "mesh";
	case AssetCookerCategory_Material:
		return "material";
	case AssetCookerCategory_Scene:
		return "scene";
	default:
		return "unknown";
	}
}

static std::string AssetCookerFeatureCapabilityValue(const SourceImportFeatureCapability& capability)
{
	return std::to_string(capability.count) + "/" + std::string(ToString(capability.support));
}

static void AssetCookerPrintImportFeatureSummary(const AssetCookerSceneEntry& sceneEntry, const SourceImportResult& importResult)
{
	const SourceImportFeatureCapabilitySummary& features = importResult.diagnostics.featureCapabilities;
	ToolConsole::Message(
	    std::cout,
	    ToolConsoleSeverity::Info,
	    "Import feature summary",
	    {ToolConsole::QuotedField("source", sceneEntry.relativePath),
	     ToolConsole::Field("importer", std::string(importResult.GetImporterId())),
	     ToolConsole::Field("animations", AssetCookerFeatureCapabilityValue(features.animations)),
	     ToolConsole::Field("cameraNodes", AssetCookerFeatureCapabilityValue(features.cameraNodes)),
	     ToolConsole::Field("lightNodes", AssetCookerFeatureCapabilityValue(features.lightNodes)),
	     ToolConsole::Field("skinnedNodes", AssetCookerFeatureCapabilityValue(features.skinnedNodes)),
	     ToolConsole::Field("weightedNodes", AssetCookerFeatureCapabilityValue(features.weightedNodes)),
	     ToolConsole::Field("morphTargets", AssetCookerFeatureCapabilityValue(features.morphTargets)),
	     ToolConsole::Field("materialVariants", AssetCookerFeatureCapabilityValue(features.materialVariants)),
	     ToolConsole::Field("meshGpuInstancing", AssetCookerFeatureCapabilityValue(features.meshGpuInstancing))});
}

static std::string AssetCookerBuildPlanStepList(const std::vector<AssetCookerPlanStep>& steps)
{
	std::ostringstream output;
	for (std::size_t stepIndex = 0; stepIndex < steps.size(); ++stepIndex)
	{
		if (stepIndex > 0u)
		{
			output << ", ";
		}
		output << AssetCookerPlanStepName(steps[stepIndex]);
	}
	return output.str();
}

static bool AssetCookerWriteTimingSummary(
    const AssetCookerProjectCookPlan& plan,
    const std::vector<AssetCookerStageTiming>& stageTimings,
    const std::vector<AssetCookerOutputRecord>& outputs,
    bool succeeded,
    std::uint64_t elapsedMilliseconds,
    std::string& outErrorMessage)
{
	std::ostringstream stages;
	stages << "[\n";
	for (std::size_t stageIndex = 0; stageIndex < stageTimings.size(); ++stageIndex)
	{
		const AssetCookerStageTiming& stage = stageTimings[stageIndex];
		stages << "    {"
		       << "\"name\": " << Json::QuoteString(stage.name) << ", "
		       << "\"elapsedMs\": " << stage.elapsedMilliseconds << ", "
		       << "\"succeeded\": " << (stage.succeeded ? "true" : "false") << "}";
		if (stageIndex + 1u < stageTimings.size())
		{
			stages << ',';
		}
		stages << "\n";
	}
	stages << "  ]";

	std::ostringstream outputRecords;
	outputRecords << "[\n";
	for (std::size_t outputIndex = 0; outputIndex < outputs.size(); ++outputIndex)
	{
		const AssetCookerOutputRecord& output = outputs[outputIndex];
		outputRecords << "    {"
		              << "\"category\": " << Json::QuoteString(AssetCookerCategoryName(output.category)) << ", "
		              << "\"assetId\": " << Json::QuoteString(output.assetId) << ", "
		              << "\"path\": " << Json::QuoteString(std::filesystem::path(output.path).generic_string()) << ", "
		              << "\"reloadHint\": " << Json::QuoteString(output.reloadHint) << "}";
		if (outputIndex + 1u < outputs.size())
		{
			outputRecords << ',';
		}
		outputRecords << "\n";
	}
	outputRecords << "  ]";

	Json::ObjectWriter writer;
	writer.WriteString("schema", "asset-cooker-summary-v1");
	writer.WriteString("tool", "AssetCooker");
	writer.WriteString("project", plan.projectName);
	writer.WriteString("configuration", plan.configuration);
	writer.WriteString("toolConfiguration", plan.toolConfiguration);
	writer.WriteString("planPath", plan.planPath.generic_string());
	writer.WriteString("textureSummaryPath", plan.textureSummaryPath.generic_string());
	writer.WriteRaw("succeeded", succeeded ? "true" : "false");
	writer.WriteUInt64("elapsedMs", elapsedMilliseconds);
	writer.WriteUInt64("engineSceneCount", static_cast<std::uint64_t>(plan.engineSceneCount));
	writer.WriteUInt64("projectSceneCount", static_cast<std::uint64_t>(plan.projectSceneCount));
	writer.WriteUInt64("overriddenEngineSceneCount", static_cast<std::uint64_t>(plan.overriddenEngineSceneCount));
	writer.WriteUInt64("sceneCount", static_cast<std::uint64_t>(plan.sceneEntries.size()));
	writer.WriteUInt64("outputCount", static_cast<std::uint64_t>(outputs.size()));
	writer.WriteRaw("stages", stages.str());
	writer.WriteRaw("outputs", outputRecords.str());

	return Files::TryWriteAllTextAtomic(plan.summaryPath, writer.Finish(), outErrorMessage);
}

static void AssetCookerPrintTopStageTimings(const std::vector<AssetCookerStageTiming>& stageTimings)
{
	std::vector<AssetCookerStageTiming> sortedTimings = stageTimings;
	std::ranges::sort(
	    sortedTimings,
	    [](const AssetCookerStageTiming& lhs, const AssetCookerStageTiming& rhs) noexcept
	    {
		    return lhs.elapsedMilliseconds > rhs.elapsedMilliseconds;
	    });

	if (sortedTimings.empty())
	{
		return;
	}

	ToolConsole::ListHeader(std::cout, "AssetCooker stage timings");
	for (std::size_t timingIndex = 0; timingIndex < sortedTimings.size(); ++timingIndex)
	{
		const AssetCookerStageTiming& timing = sortedTimings[timingIndex];
		ToolConsole::ListItem(
		    std::cout,
		    timingIndex + 1u,
		    {ToolConsole::Field("stage", timing.name),
		     ToolConsole::Field("elapsedMs", std::to_string(timing.elapsedMilliseconds)),
		     ToolConsole::Field("status", timing.succeeded ? "succeeded" : "failed")});
	}
}

static bool AssetCookerFileExists(const std::filesystem::path& path)
{
	std::error_code errorCode;
	return std::filesystem::exists(path, errorCode);
}

static std::shared_ptr<spdlog::logger> AssetCookerGetLogger()
{
	static const auto logger = Logging::GetOrCreateLogger("Tools.AssetCooker");
	return logger;
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
	return plan.repositoryRoot / "artifacts" / "diagnostics" / "cook" / "Temp" /
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
	const std::string scopeName = "Tools.AssetCooker.ImportScene." + sceneEntry.relativePath;
	SPARKLE_LOG_SCOPE(AssetCookerGetLogger(), spdlog::level::debug, scopeName);

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

	AssetCookerPrintImportFeatureSummary(sceneEntry, importResult);

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
	const std::string scopeName = "Tools.AssetCooker.CookImportedScene." + sceneEntry.relativePath;
	SPARKLE_LOG_SCOPE(AssetCookerGetLogger(), spdlog::level::info, scopeName);

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
	     ToolConsole::Field("importSourceMeshes", std::to_string(importResult.diagnostics.summary.sourceMeshCount)),
	     ToolConsole::Field("importSourceMaterials", std::to_string(importResult.diagnostics.summary.sourceMaterialCount)),
	     ToolConsole::Field("importTextureBindings", std::to_string(importResult.diagnostics.textures.resolvedTextureBindingCount)),
	     ToolConsole::Field("importWarnings", std::to_string(importResult.diagnostics.issues.warningMessageCount)),
	     ToolConsole::Field("importedMeshPrimitives", std::to_string(importResult.diagnostics.summary.importedMeshPrimitiveCount)),
	     ToolConsole::Field("importedMeshInstances", std::to_string(importResult.diagnostics.summary.importedMeshInstanceCount)),
	     ToolConsole::Field("importedMeshInstanceGroups", std::to_string(importResult.diagnostics.summary.importedMeshInstanceGroupCount)),
	     ToolConsole::Field(
	         "importUniqueMeshPrimitiveCandidates",
	         std::to_string(importResult.diagnostics.geometryInstancing.uniqueMeshPrimitiveCandidateCount)),
	     ToolConsole::Field("importMeshPlacements", std::to_string(importResult.diagnostics.geometryInstancing.meshPlacementCount)),
	     ToolConsole::Field(
	         "importAuthoredInstanceGroups",
	         std::to_string(importResult.diagnostics.geometryInstancing.authoredInstanceGroupCount)),
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
	SPARKLE_LOG_SCOPE(AssetCookerGetLogger(), spdlog::level::info, "AssetCooker.CollectTextureRequests");

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
	SPARKLE_LOG_SCOPE(AssetCookerGetLogger(), spdlog::level::info, "AssetCooker.Stage.Shaders");

	const std::filesystem::path shaderCompilerPath = AssetCookerResolveToolPath(plan, "ShaderCompiler");
	ToolConsole::Info("Cooking shaders: validating package registry...");
	int exitCode = AssetCookerToolProcess::Run(shaderCompilerPath, {L"list-shaders", L"--validate"}, plan.projectRoot);
	if (exitCode != 0)
	{
		diagnostics.AddError(AssetCookerCategory_Shaders, "Shader registration validation failed.");
		return false;
	}

	ToolConsole::Info("Cooking shaders: writing package payloads...");
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
	SPARKLE_LOG_SCOPE(AssetCookerGetLogger(), spdlog::level::info, "AssetCooker.Stage.Textures");

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
	    {L"cook-request-file", textureRequestPath.wstring(), L"--summary", plan.textureSummaryPath.wstring()},
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
	SPARKLE_LOG_SCOPE(AssetCookerGetLogger(), spdlog::level::info, "AssetCooker.Stage.SceneAssets");

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
	SPARKLE_LOG_SCOPE(AssetCookerGetLogger(), spdlog::level::info, "AssetCooker.DispatchPlan");
	const auto dispatchStartTime = std::chrono::steady_clock::now();
	std::vector<AssetCookerStageTiming> stageTimings;

	Filesystem::ConfigureProjectRoot(plan.projectRoot);

	ToolConsole::Summary(
	    std::cout,
	    "AssetCooker plan",
	    {ToolConsole::QuotedField("project", plan.projectName),
	     ToolConsole::QuotedField("configuration", plan.configuration),
	     ToolConsole::QuotedField("toolConfiguration", plan.toolConfiguration),
	     ToolConsole::QuotedField("steps", AssetCookerBuildPlanStepList(plan.steps)),
	     ToolConsole::Field("scenes", std::to_string(plan.sceneEntries.size())),
	     ToolConsole::Field("engineScenes", std::to_string(plan.engineSceneCount)),
	     ToolConsole::Field("projectScenes", std::to_string(plan.projectSceneCount)),
	     ToolConsole::Field("overrides", std::to_string(plan.overriddenEngineSceneCount))});

	for (std::size_t stepIndex = 0; stepIndex < plan.steps.size(); ++stepIndex)
	{
		const AssetCookerPlanStep step = plan.steps[stepIndex];
		AssetCookerStageTiming stageTiming;
		stageTiming.name = AssetCookerPlanStepName(step);
		const auto stageStartTime = std::chrono::steady_clock::now();
		ToolConsole::Progress(std::cout, "Cooking", "stage", stepIndex + 1u, plan.steps.size(), stageTiming.name);

		if (step == AssetCookerPlanStep::Shaders)
		{
			stageTiming.succeeded = AssetCookerRunShaders(plan, diagnostics, outOutputs);
		}
		else if (step == AssetCookerPlanStep::Textures)
		{
			stageTiming.succeeded = AssetCookerRunTextures(plan, diagnostics, outOutputs);
		}
		else if (step == AssetCookerPlanStep::SceneAssets)
		{
			stageTiming.succeeded = AssetCookerRunSceneAssets(plan, diagnostics, outOutputs);
		}

		stageTiming.elapsedMilliseconds = AssetCookerElapsedMilliseconds(stageStartTime);
		stageTimings.push_back(std::move(stageTiming));
		ToolConsole::Message(
		    std::cout,
		    ToolConsoleSeverity::Info,
		    "Stage finished",
		    {ToolConsole::QuotedField("name", stageTimings.back().name),
		     ToolConsole::Field("status", stageTimings.back().succeeded ? "completed" : "failed"),
		     ToolConsole::Field("elapsedMs", std::to_string(stageTimings.back().elapsedMilliseconds))});
		if (!stageTimings.back().succeeded)
		{
			AssetCookerPrintTopStageTimings(stageTimings);
			std::string summaryError;
			if (!AssetCookerWriteTimingSummary(
			        plan,
			        stageTimings,
			        outOutputs,
			        false,
			        AssetCookerElapsedMilliseconds(dispatchStartTime),
			        summaryError))
			{
				diagnostics.AddWarning(AssetCookerCategory_All, "Failed to write timing summary: " + summaryError);
			}
			else
			{
				diagnostics.AddInfo(AssetCookerCategory_All, "Timing summary written to " + plan.summaryPath.string() + ".");
			}
			return false;
		}
	}

	AssetCookerPrintTopStageTimings(stageTimings);
	std::string summaryError;
	if (!AssetCookerWriteTimingSummary(
	        plan,
	        stageTimings,
	        outOutputs,
	        true,
	        AssetCookerElapsedMilliseconds(dispatchStartTime),
	        summaryError))
	{
		diagnostics.AddWarning(AssetCookerCategory_All, "Failed to write timing summary: " + summaryError);
	}
	else
	{
		diagnostics.AddInfo(AssetCookerCategory_All, "Timing summary written to " + plan.summaryPath.string() + ".");
	}

	return true;
}
