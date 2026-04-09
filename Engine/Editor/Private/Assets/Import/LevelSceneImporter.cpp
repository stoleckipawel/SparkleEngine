#include "PCH.h"

#include "Assets/Import/LevelSceneImporter.h"

#include "Assets/Import/SceneImporter.h"
#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Diagnostics/Log.h"
#include "Level/Level.h"
#include "Level/LevelChangeEvents.h"
#include "Level/LevelManager.h"
#include "Scene/GameScene.h"

#include <utility>

LevelSceneImporter::LevelSceneImporter(LevelManager& levelManager, GameScene& gameScene) noexcept :
	m_levelManager(&levelManager),
	m_gameScene(&gameScene)
{
	auto handle = m_levelManager->GetLevelChangeEvents().OnLevelChanged.Add(
	    [this](const LevelChangedEventArgs& args)
	    {
		    HandleLevelChanged(args);
	    });
	m_levelChangedHandle = ScopedEventHandle(m_levelManager->GetLevelChangeEvents().OnLevelChanged, handle);

	ImportActiveLevelIfPresent();
}

ImportedSceneData LevelSceneImporter::BuildImportedSceneData(SceneImportResult&& importResult)
{
	ImportedSceneData importedSceneData;
	importedSceneData.meshes = std::move(importResult.meshes);
	importedSceneData.materials = std::move(importResult.materials);
	importedSceneData.transforms = std::move(importResult.transforms);
	importedSceneData.materialHandles = std::move(importResult.materialHandles);
	return importedSceneData;
}

void LevelSceneImporter::ImportActiveLevelIfPresent() noexcept
{
	if (m_levelManager == nullptr)
	{
		return;
	}

	const LevelAsset* activeLevel = m_levelManager->GetActiveLevel();
	if (activeLevel == nullptr)
	{
		return;
	}

	ImportLevel(*activeLevel);
}

void LevelSceneImporter::HandleLevelChanged(const LevelChangedEventArgs& args) noexcept
{
	if (m_levelManager == nullptr)
	{
		return;
	}

	const LevelAsset* activeLevel = m_levelManager->GetActiveLevel();
	if (activeLevel == nullptr)
	{
		LOG_WARNING("LevelSceneImporter: Active level is unavailable after level change to '" + args.activeLevelName + "'");
		return;
	}

	ImportLevel(*activeLevel);
}

bool LevelSceneImporter::ImportLevel(const LevelAsset& level) noexcept
{
	if (m_gameScene == nullptr)
	{
		return false;
	}

	const LevelDesc& levelDesc = level.GetLevelDesc();
	if (levelDesc.importedMeshRequests.empty())
	{
		return true;
	}

	bool importedAnyMeshes = false;
	for (const ImportedMeshRequest& request : levelDesc.importedMeshRequests)
	{
		importedAnyMeshes = ImportMeshRequest(request) || importedAnyMeshes;
	}

	if (!importedAnyMeshes)
	{
		LOG_WARNING(
		    "LevelSceneImporter: Level '" + std::string(level.GetName()) + "' did not produce any imported meshes from its source asset requests");
	}

	return importedAnyMeshes;
}

bool LevelSceneImporter::ImportMeshRequest(const ImportedMeshRequest& request) noexcept
{
	if (m_gameScene == nullptr)
	{
		return false;
	}

	const std::optional<std::filesystem::path> resolvedAssetPath = Filesystem::ResolveAssetPath(request.assetPath, AssetType::Mesh);
	if (!resolvedAssetPath)
	{
		LOG_WARNING("LevelSceneImporter: Mesh asset not found — " + request.assetPath.string());
		return false;
	}

	SceneImportResult importResult = SceneImporter::Import(*resolvedAssetPath);
	if (!importResult.IsValid())
	{
		LOG_WARNING("LevelSceneImporter: Failed to import mesh asset '" + resolvedAssetPath->string() + "'");
		return false;
	}

	ImportedSceneData importedSceneData = BuildImportedSceneData(std::move(importResult));
	if (!m_gameScene->AppendImportedSceneData(std::move(importedSceneData)))
	{
		LOG_WARNING("LevelSceneImporter: Imported mesh asset produced no appendable scene content '" + resolvedAssetPath->string() + "'");
		return false;
	}

	return true;
}