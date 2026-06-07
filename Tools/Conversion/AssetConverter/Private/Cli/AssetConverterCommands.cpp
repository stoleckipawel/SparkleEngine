#include "PCH.h"

#include "Cli/AssetConverterCommands.h"

#include "CookedMeshAssetBuilder.h"
#include "CookedMeshAssetWriter.h"
#include "MaterialCooker.h"
#include "SceneCooker.h"
#include "SourceSceneImporter.h"

#include <iostream>
#include <utility>
#include <vector>

#include <objbase.h>

static void PrintFeatureCapability(
    std::ostream& output,
    std::string_view name,
    const SourceImportFeatureCapability& capability)
{
	output << " " << name << "=" << capability.count << "/" << ToString(capability.support);
}

void AssetConverterCommands::PrintCookSceneSummary(
    const std::filesystem::path& sourceScenePath,
    const SourceImportResult& importResult,
    const CookedSceneBuild& cookedSceneBuild)
{
	std::cout << "AssetConverter Summary:\n"
	          << "  mode=cook-scene\n"
	          << "  source='" << sourceScenePath.string() << "'\n"
	          << "  importer='" << importResult.GetImporterName() << "'\n"
	          << "  meshPrimitives=" << importResult.GetMeshPrimitiveCount() << "\n"
	          << "  meshInstances=" << importResult.GetMeshInstanceCount() << "\n"
	          << "  meshInstanceGroups=" << importResult.GetMeshInstanceGroupCount() << "\n"
	          << "  cameras=" << importResult.GetCameraCount() << "\n"
	          << "  lights=" << importResult.GetLightCount() << "\n"
	          << "  materials=" << importResult.GetMaterialCount() << "\n"
	          << "  cookedMeshAssetRefs=" << cookedSceneBuild.manifest.meshAssetReferences.size() << "\n"
	          << "  cookedInstances=" << cookedSceneBuild.manifest.instances.size() << "\n"
	          << "  cookedInstanceGroups=" << cookedSceneBuild.manifest.instanceGroups.size() << "\n"
	          << "  cookedCameras=" << cookedSceneBuild.manifest.cameras.size() << "\n"
	          << "  cookedLights=" << cookedSceneBuild.manifest.lights.size() << "\n"
	          << "  sceneManifest='" << cookedSceneBuild.identity.manifestPath.string() << "'\n";
}

void AssetConverterCommands::PrintImportFeatureSummary(
    const std::filesystem::path& sourceScenePath,
    const SourceImportResult& importResult)
{
	const SourceImportFeatureCapabilitySummary& features = importResult.diagnostics.featureCapabilities;
	std::cout << "AssetConverter Import Features: source='" << sourceScenePath.string() << "'";
	PrintFeatureCapability(std::cout, "animations", features.animations);
	PrintFeatureCapability(std::cout, "cameraNodes", features.cameraNodes);
	PrintFeatureCapability(std::cout, "lightNodes", features.lightNodes);
	PrintFeatureCapability(std::cout, "skinnedNodes", features.skinnedNodes);
	PrintFeatureCapability(std::cout, "weightedNodes", features.weightedNodes);
	PrintFeatureCapability(std::cout, "morphTargets", features.morphTargets);
	PrintFeatureCapability(std::cout, "materialVariants", features.materialVariants);
	PrintFeatureCapability(std::cout, "meshGpuInstancing", features.meshGpuInstancing);
	std::cout << "\n";
}

void AssetConverterCommands::PrintCollectTextureSummary(
    const std::filesystem::path& sourceScenePath,
    std::size_t requestCount,
    const std::filesystem::path& outputRequestPath)
{
	std::cout << "AssetConverter Summary:\n"
	          << "  mode=collect-texture-requests\n"
	          << "  source='" << sourceScenePath.string() << "'\n"
	          << "  uniqueRequests=" << requestCount << "\n"
	          << "  requestFile='" << outputRequestPath.string() << "'\n";
}

int AssetConverterCommands::RunWithImportedScene(
    const std::filesystem::path& sourceScenePath,
    const std::function<int(const SourceImportResult&)>& onImportedScene)
{
	const HRESULT coInitializeResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	if (FAILED(coInitializeResult) && coInitializeResult != RPC_E_CHANGED_MODE)
	{
		std::cerr << "AssetConverter: failed to initialize COM for source texture loading\n";
		return 4;
	}

	SourceImportResult importResult = SourceSceneImporter::Import(sourceScenePath);
	if (!importResult.IsValid())
	{
		if (SUCCEEDED(coInitializeResult))
		{
			CoUninitialize();
		}

		std::cerr << "AssetConverter: failed to import '" << sourceScenePath.string() << "'\n";
		return 2;
	}

	PrintImportFeatureSummary(sourceScenePath, importResult);

	const int exitCode = onImportedScene(importResult);

	if (SUCCEEDED(coInitializeResult))
	{
		CoUninitialize();
	}

	return exitCode;
}

CookedSceneBuild AssetConverterCommands::CookImportedScene(
    const std::filesystem::path& sourceScenePath,
    const SourceImportResult& importResult)
{
	CookedSceneBuild build;
	if (!importResult.IsValid())
	{
		build.status.errorMessage = "Scene import result is not valid";
		return build;
	}

	if (!SceneCooker::ResolveSceneIdentity(sourceScenePath, build.identity, build.status.errorMessage))
	{
		return build;
	}

	build.ApplyMeshOutput(CookedMeshAssetBuilder::BuildMeshAssets(importResult, build.identity.assetId));
	MaterialCookOutput materialOutput;
	if (!MaterialCooker::BuildMaterialAssets(importResult, build.identity.assetId, materialOutput, build.status.errorMessage))
	{
		return build;
	}
	build.ApplyMaterialOutput(std::move(materialOutput));

	if (!SceneCooker::BuildManifest(importResult, build, build.status.errorMessage))
	{
		return build;
	}

	if (!CookedMeshAssetWriter::WriteMeshAssets(build.outputs.meshAssets, build.status.errorMessage) ||
	    !MaterialCooker::WriteMaterialAssets(build.outputs.materialAssets, build.status.errorMessage) ||
	    !SceneCooker::WriteSceneManifestAndRegistry(build, build.status.errorMessage))
	{
		return build;
	}

	return build;
}

int AssetConverterCommands::RunCookScene(const std::filesystem::path& sourceScenePath)
{
	return RunWithImportedScene(
	    sourceScenePath,
	    [&](const SourceImportResult& importResult) -> int
	    {
		    const CookedSceneBuild cookedSceneBuild = CookImportedScene(sourceScenePath, importResult);
		    if (!cookedSceneBuild.Succeeded())
		    {
			    std::cerr << "AssetConverter: failed to cook '" << sourceScenePath.string() << "' - "
			              << cookedSceneBuild.status.errorMessage << "\n";
			    return 3;
		    }

		    std::cout << "AssetConverter: imported '" << sourceScenePath.string() << "' via "
		              << importResult.GetImporterName() << " with " << importResult.GetMeshPrimitiveCount()
		              << " mesh primitives, " << importResult.GetMeshInstanceCount() << " mesh instances, "
		              << importResult.GetMeshInstanceGroupCount() << " mesh instance groups, and "
		              << importResult.GetMaterialCount() << " materials; emitted scene asset '"
		              << cookedSceneBuild.identity.assetId << "' to '" << cookedSceneBuild.identity.manifestPath.string() << "'\n";
		    PrintCookSceneSummary(sourceScenePath, importResult, cookedSceneBuild);

		    return 0;
	    });
}

int AssetConverterCommands::RunCollectTextureRequests(
    const std::filesystem::path& sourceScenePath,
    const std::filesystem::path& outputRequestPath)
{
	return RunWithImportedScene(
	    sourceScenePath,
	    [&](const SourceImportResult& importResult) -> int
	    {
		    std::vector<TextureCookRequest> requests;
		    std::string errorMessage;
		    if (!MaterialCooker::CollectTextureCookRequests(importResult, requests, errorMessage))
		    {
			    std::cerr << "AssetConverter: failed to collect texture requests for '" << sourceScenePath.string() << "' - "
			              << errorMessage << "\n";
			    return 5;
		    }

		    if (!WriteTextureCookRequestList(outputRequestPath, requests, errorMessage))
		    {
			    std::cerr << "AssetConverter: failed to write texture request file '" << outputRequestPath.string() << "' - "
			              << errorMessage << "\n";
			    return 6;
		    }

		    std::cout << "AssetConverter: collected " << requests.size() << " unique texture request(s) from '"
		              << sourceScenePath.string() << "' into '" << outputRequestPath.string() << "'\n";
		    PrintCollectTextureSummary(sourceScenePath, requests.size(), outputRequestPath);
		    return 0;
	    });
}



