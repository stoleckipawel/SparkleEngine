#include "ImportedSceneCooker.h"

#include "CookedMeshAssetBuilder.h"
#include "MaterialCooker.h"
#include "SceneCooker.h"
#include "SourceSceneImporter.h"
#include "ToolConsole.h"

#include <iostream>
#include <string>
#include <utility>

#include <objbase.h>

class ImportedSceneComApartment final
{
  public:
	ImportedSceneComApartment() noexcept;
	~ImportedSceneComApartment();

	ImportedSceneComApartment(const ImportedSceneComApartment&) = delete;
	ImportedSceneComApartment& operator=(const ImportedSceneComApartment&) = delete;

	bool CanImport() const noexcept;

  private:
	HRESULT m_result;
};

ImportedSceneComApartment::ImportedSceneComApartment() noexcept :
    m_result(CoInitializeEx(nullptr, COINIT_MULTITHREADED))
{
}

ImportedSceneComApartment::~ImportedSceneComApartment()
{
	if (SUCCEEDED(m_result))
	{
		CoUninitialize();
	}
}

bool ImportedSceneComApartment::CanImport() const noexcept
{
	return SUCCEEDED(m_result) || m_result == RPC_E_CHANGED_MODE;
}

bool ImportedSceneCooker::Import(
    const AssetCookerSceneEntry& sceneEntry,
    AssetCookerCategory category,
    AssetCookerDiagnostics& diagnostics,
    SourceImportResult& outImport)
{
	const ImportedSceneComApartment comApartment;
	if (!comApartment.CanImport())
	{
		diagnostics.AddError(category, "Failed to initialize COM for source import.", sceneEntry.sourcePath);
		return false;
	}

	outImport = SourceSceneImporter::Import(sceneEntry.sourcePath);
	if (outImport.IsValid())
	{
		return true;
	}

	diagnostics.AddError(
	    category,
	    "Failed to import source scene with importer '" + std::string(outImport.GetImporterId()) + "'.",
	    outImport.GetSourcePath().empty() ? sceneEntry.sourcePath : outImport.GetSourcePath());
	return false;
}

bool ImportedSceneCooker::Build(
    const AssetCookerSceneEntry& sceneEntry,
    AssetCookerDiagnostics& diagnostics,
    ImportedSceneCookProduct& outProduct)
{
	outProduct = {};
	if (!Import(
	        sceneEntry,
	        AssetCookerCategory_SceneAssets,
	        diagnostics,
	        outProduct.Import))
	{
		return false;
	}

	return BuildCookedScene(sceneEntry, outProduct.Import, outProduct.Scene, diagnostics);
}

bool ImportedSceneCooker::BuildCookedScene(
    const AssetCookerSceneEntry& sceneEntry,
    const SourceImportResult& importResult,
    CookedSceneBuild& build,
    AssetCookerDiagnostics& diagnostics)
{
	if (!SceneCooker::ResolveSceneIdentity(
	        sceneEntry.sourcePath,
	        build.identity,
	        build.status.errorMessage))
	{
		diagnostics.AddError(
		    AssetCookerCategory_SceneAssets,
		    build.status.errorMessage,
		    sceneEntry.sourcePath);
		return false;
	}

	build.ApplyMeshOutput(
	    CookedMeshAssetBuilder::BuildMeshAssets(
	        importResult,
	        build.identity.assetId));

	MaterialCookOutput materialOutput;
	if (!MaterialCooker::BuildMaterialAssets(
	        importResult,
	        build.identity.assetId,
	        materialOutput,
	        build.status.errorMessage))
	{
		diagnostics.AddError(
		    AssetCookerCategory_Materials,
		    build.status.errorMessage,
		    sceneEntry.sourcePath);
		return false;
	}

	build.ApplyMaterialOutput(std::move(materialOutput));
	if (!SceneCooker::BuildManifest(
	        importResult,
	        build,
	        build.status.errorMessage))
	{
		diagnostics.AddError(
		    AssetCookerCategory_SceneAssets,
		    build.status.errorMessage,
		    sceneEntry.sourcePath);
		return false;
	}

	return true;
}

void ImportedSceneCooker::Report(
    const AssetCookerSceneEntry& sceneEntry,
    const ImportedSceneCookProduct& product)
{
	const SourceImportResult& importResult = product.Import;
	const CookedSceneBuild& build = product.Scene;

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
	     ToolConsole::Field(
	         "materialVariantMappings",
	         std::to_string(importResult.GetMaterialVariantMappingCount())),
	     ToolConsole::Field(
	         "cookedMeshAssetRefs",
	         std::to_string(build.manifest.meshAssetReferences.size())),
	     ToolConsole::Field("cookedInstances", std::to_string(build.manifest.instances.size())),
	     ToolConsole::Field(
	         "cookedInstanceGroups",
	         std::to_string(build.manifest.instanceGroups.size())),
	     ToolConsole::Field(
	         "cookedMaterialVariants",
	         std::to_string(build.manifest.materialVariants.size())),
	     ToolConsole::Field(
	         "cookedVariantMappings",
	         std::to_string(build.manifest.materialVariantMappings.size())),
	     ToolConsole::Field("cookedCameras", std::to_string(build.manifest.cameras.size())),
	     ToolConsole::Field("cookedLights", std::to_string(build.manifest.lights.size())),
	     ToolConsole::PathField("manifest", build.identity.manifestPath)});
}
