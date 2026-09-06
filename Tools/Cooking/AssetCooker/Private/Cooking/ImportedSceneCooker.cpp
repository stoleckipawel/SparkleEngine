#include "ImportedSceneCooker.h"

#include "CookedMeshAssetBuilder.h"
#include "MaterialCooker.h"
#include "SceneCooker.h"
#include "Core/Public/Diagnostics/Error.h"
#include "SourceSceneImporter.h"

#include <string>

#include <objbase.h>

class ImportedSceneComApartment final
{
public:
	ImportedSceneComApartment();
	~ImportedSceneComApartment();

	ImportedSceneComApartment(const ImportedSceneComApartment&) = delete;
	ImportedSceneComApartment& operator=(const ImportedSceneComApartment&) = delete;

private:
	HRESULT m_result;
};

ImportedSceneComApartment::ImportedSceneComApartment() :
    m_result(CoInitializeEx(nullptr, COINIT_MULTITHREADED))
{
	if (!SUCCEEDED(m_result) && m_result != RPC_E_CHANGED_MODE)
	{
		throw Diagnostics::Error("Failed to initialize COM for source import.");
	}
}

ImportedSceneComApartment::~ImportedSceneComApartment()
{
	if (SUCCEEDED(m_result))
	{
		CoUninitialize();
	}
}

SourceImportOutput ImportedSceneCooker::Import(const AssetCookerSceneEntry& sceneEntry)
{
	const ImportedSceneComApartment comApartment;
	return SourceSceneImporter::Import(sceneEntry.sourcePath);
}

CookedSceneBuild ImportedSceneCooker::Build(const AssetCookerSceneEntry& sceneEntry, AssetCookerDiagnostics& diagnostics)
{
	SourceImportOutput importOutput;
	try
	{
		importOutput = Import(sceneEntry);
	}
	catch (const Diagnostics::Error& error)
	{
		diagnostics.AddError(AssetCookerCategory::SceneAssets, error.what(), sceneEntry.sourcePath);
		throw;
	}
	return BuildCookedScene(sceneEntry, importOutput, diagnostics);
}

CookedSceneBuild ImportedSceneCooker::BuildCookedScene(
    const AssetCookerSceneEntry& sceneEntry,
    const SourceImportOutput& importOutput,
    AssetCookerDiagnostics& diagnostics)
{
	CookedSceneBuild build;
	try
	{
		build.identity = SceneCooker::ResolveSceneIdentity(sceneEntry.sourcePath);
	}
	catch (const Diagnostics::Error& error)
	{
		diagnostics.AddError(AssetCookerCategory::SceneAssets, error.what(), sceneEntry.sourcePath);
		throw;
	}
	try
	{
		build.ApplyMeshOutput(CookedMeshAssetBuilder::BuildMeshAssets(importOutput, build.identity.assetId));
	}
	catch (const Diagnostics::Error& error)
	{
		diagnostics.AddError(AssetCookerCategory::Meshes, error.what(), sceneEntry.sourcePath);
		throw;
	}

	try
	{
		build.ApplyMaterialOutput(MaterialCooker::BuildMaterialAssets(importOutput, build.identity.assetId));
	}
	catch (const Diagnostics::Error& error)
	{
		diagnostics.AddError(AssetCookerCategory::Materials, error.what(), sceneEntry.sourcePath);
		throw;
	}

	try
	{
		SceneCooker::BuildManifest(importOutput, build);
	}
	catch (const Diagnostics::Error& error)
	{
		diagnostics.AddError(AssetCookerCategory::SceneAssets, error.what(), sceneEntry.sourcePath);
		throw;
	}
	return build;
}
