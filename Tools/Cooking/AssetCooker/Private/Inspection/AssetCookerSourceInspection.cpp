#include "AssetCookerSourceInspection.h"

#include "MaterialCooker.h"
#include "SourceSceneImporter.h"
#include "TextureCookRequestList.h"
#include "ToolConsole.h"

#include "Core/Public/FileSystemUtils.h"

#include <iostream>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <objbase.h>

class AssetCookerSourceInspectionOperations final
{
  public:
	static void PrintImportFeatureSummary(const std::filesystem::path& sourceScenePath, const SourceImportResult& importResult)
	{
		ToolConsole::Summary(
		    std::cout,
		    "Source import inspection",
		    {ToolConsole::PathField("source", sourceScenePath),
		     ToolConsole::Field("importer", std::string(importResult.GetImporterId())),
		     ToolConsole::Field("meshPrimitives", std::to_string(importResult.GetMeshPrimitiveCount())),
		     ToolConsole::Field("meshInstances", std::to_string(importResult.GetMeshInstanceCount())),
		     ToolConsole::Field("meshInstanceGroups", std::to_string(importResult.GetMeshInstanceGroupCount())),
		     ToolConsole::Field("cameras", std::to_string(importResult.GetCameraCount())),
		     ToolConsole::Field("lights", std::to_string(importResult.GetLightCount())),
		     ToolConsole::Field("materials", std::to_string(importResult.GetMaterialCount())),
		     ToolConsole::Field("materialVariants", std::to_string(importResult.GetMaterialVariantCount())),
		     ToolConsole::Field("materialVariantMappings", std::to_string(importResult.GetMaterialVariantMappingCount())),
		     ToolConsole::Field("animations", std::to_string(importResult.GetAnimationCount()))});
	}

	static void ConfigureProjectContextForSource(const std::filesystem::path& sourceScenePath)
	{
		const std::filesystem::path searchRoot = sourceScenePath.has_parent_path() ? sourceScenePath.parent_path() : std::filesystem::current_path();
		if (const std::optional<std::filesystem::path> projectRoot = Filesystem::FindAncestorWithMarker(searchRoot, Filesystem::kProjectMarker))
		{
			Filesystem::ConfigureProjectRoot(*projectRoot);
		}
	}

	template <typename ImportedSceneHandler>
	static int RunWithImportedScene(const std::filesystem::path& sourceScenePath, ImportedSceneHandler&& importedSceneHandler)
	{
		ConfigureProjectContextForSource(sourceScenePath);

		const HRESULT coInitializeResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
		if (FAILED(coInitializeResult) && coInitializeResult != RPC_E_CHANGED_MODE)
		{
			ToolConsole::Message(
			    std::cerr,
			    ToolConsoleSeverity::Error,
			    "Failed to initialize host texture loading.",
			    {ToolConsole::PathField("source", sourceScenePath)});
			return 4;
		}

		SourceImportResult importResult = SourceSceneImporter::Import(sourceScenePath);
		if (!importResult.IsValid())
		{
			if (SUCCEEDED(coInitializeResult))
			{
				CoUninitialize();
			}

			ToolConsole::Message(
			    std::cerr,
			    ToolConsoleSeverity::Error,
			    "Failed to import source scene.",
			    {ToolConsole::PathField("source", sourceScenePath),
			     ToolConsole::Field("importer", std::string(importResult.GetImporterId()))});
			return 2;
		}

		PrintImportFeatureSummary(sourceScenePath, importResult);

		const int exitCode = importedSceneHandler(importResult);
		if (SUCCEEDED(coInitializeResult))
		{
			CoUninitialize();
		}

		return exitCode;
	}
};

int AssetCookerSourceInspection::InspectSource(const std::filesystem::path& sourceScenePath)
{
	return AssetCookerSourceInspectionOperations::RunWithImportedScene(
	    sourceScenePath,
	    [](const SourceImportResult&) -> int
	    {
		    return 0;
	    });
}

int AssetCookerSourceInspection::CollectTextureRequests(
    const std::filesystem::path& sourceScenePath,
    const std::filesystem::path& outputRequestPath)
{
	return AssetCookerSourceInspectionOperations::RunWithImportedScene(
	    sourceScenePath,
	    [&](const SourceImportResult& importResult) -> int
	    {
		    std::vector<TextureCookRequest> requests;
		    std::string errorMessage;
		    if (!MaterialCooker::CollectTextureCookRequests(importResult, requests, errorMessage))
		    {
			    ToolConsole::Message(
			        std::cerr,
			        ToolConsoleSeverity::Error,
			        "Failed to collect source texture requests.",
			        {ToolConsole::PathField("source", sourceScenePath), ToolConsole::Field("reason", errorMessage)});
			    return 5;
		    }

		    if (!WriteTextureCookRequestList(outputRequestPath, requests, errorMessage))
		    {
			    ToolConsole::Message(
			        std::cerr,
			        ToolConsoleSeverity::Error,
			        "Failed to write texture request file.",
			        {ToolConsole::PathField("source", sourceScenePath),
			         ToolConsole::PathField("output", outputRequestPath),
			         ToolConsole::Field("reason", errorMessage)});
			    return 6;
		    }

		    ToolConsole::Summary(
		        std::cout,
		        "Texture request inspection",
		        {ToolConsole::PathField("source", sourceScenePath),
		         ToolConsole::Field("textures", std::to_string(requests.size())),
		         ToolConsole::PathField("output", outputRequestPath)});
		    return 0;
	    });
}
