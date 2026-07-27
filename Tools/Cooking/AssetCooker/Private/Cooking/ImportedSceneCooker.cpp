#include "ImportedSceneCooker.h"

#include "CookedMeshAssetBuilder.h"
#include "MaterialCooker.h"
#include "SceneCooker.h"
#include "SourceSceneImporter.h"

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
	SourceImportResult importResult;
	if (!Import(
	        sceneEntry,
	        AssetCookerCategory_SceneAssets,
	        diagnostics,
	        importResult))
	{
		return false;
	}

	return BuildCookedScene(
	    sceneEntry,
	    importResult,
	    outProduct.Scene,
	    diagnostics);
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
