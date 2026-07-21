#include "ImportedSceneCooker.h"

#include "CookedAnimationAssetWriter.h"
#include "CookedMeshAssetBuilder.h"
#include "CookedMeshAssetWriter.h"
#include "CookedSkeletonAssetWriter.h"
#include "MaterialCooker.h"
#include "SceneCooker.h"
#include "SourceSceneImporter.h"
#include "ToolConsole.h"

#include <iostream>
#include <string>
#include <utility>

#include <objbase.h>

namespace
{
	class ComApartmentScope final
	{
	  public:
		ComApartmentScope() noexcept : m_result(CoInitializeEx(nullptr, COINIT_MULTITHREADED)) {}
		~ComApartmentScope()
		{
			if (SUCCEEDED(m_result))
			{
				CoUninitialize();
			}
		}

		bool CanImport() const noexcept { return SUCCEEDED(m_result) || m_result == RPC_E_CHANGED_MODE; }

		ComApartmentScope(const ComApartmentScope&) = delete;
		ComApartmentScope& operator=(const ComApartmentScope&) = delete;

	  private:
		HRESULT m_result;
	};

	bool BuildCookedScene(
	    const AssetCookerSceneEntry& sceneEntry,
	    const SourceImportResult& importResult,
	    CookedSceneBuild& build,
	    AssetCookerDiagnostics& diagnostics)
	{
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
		return true;
	}

	bool WriteCookedScene(
	    const AssetCookerSceneEntry& sceneEntry, CookedSceneBuild& build, AssetCookerDiagnostics& diagnostics)
	{
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
		return true;
	}

	void ReportCookedScene(
	    const AssetCookerSceneEntry& sceneEntry,
	    const SourceImportResult& importResult,
	    const CookedSceneBuild& build)
	{
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
	}
}

bool ImportedSceneCooker::ImportAndVisit(
    const AssetCookerSceneEntry& sceneEntry,
    AssetCookerCategory category,
    AssetCookerDiagnostics& diagnostics,
    const SceneVisitor& visitor)
{
	const ComApartmentScope comApartment;
	if (!comApartment.CanImport())
	{
		diagnostics.AddError(category, "Failed to initialize COM for source import.", sceneEntry.sourcePath);
		return false;
	}

	const SourceImportResult importResult = SourceSceneImporter::Import(sceneEntry.sourcePath);
	if (!importResult.IsValid())
	{
		diagnostics.AddError(
		    category,
		    "Failed to import source scene with importer '" + std::string(importResult.GetImporterId()) + "'.",
		    importResult.GetSourcePath().empty() ? sceneEntry.sourcePath : importResult.GetSourcePath());
		return false;
	}
	return visitor(importResult);
}

bool ImportedSceneCooker::Cook(
    const AssetCookerSceneEntry& sceneEntry,
    const SourceImportResult& importResult,
    AssetCookerDiagnostics& diagnostics)
{
	if (!importResult.IsValid())
	{
		diagnostics.AddError(AssetCookerCategory_SceneAssets, "Scene import result is not valid.", sceneEntry.sourcePath);
		return false;
	}

	CookedSceneBuild build;
	if (!BuildCookedScene(sceneEntry, importResult, build, diagnostics) || !WriteCookedScene(sceneEntry, build, diagnostics))
	{
		return false;
	}
	ReportCookedScene(sceneEntry, importResult, build);
	return true;
}
