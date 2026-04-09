#pragma once

#include "EditorAPI.h"

#include "Events/ScopedEventHandle.h"

class GameScene;
class LevelAsset;
class LevelManager;
struct ImportedSceneData;
struct ImportedMeshRequest;
struct LevelChangedEventArgs;
struct SceneImportResult;

class SPARKLE_EDITOR_API LevelSceneImporter final
{
  public:
	LevelSceneImporter(LevelManager& levelManager, GameScene& gameScene) noexcept;
	~LevelSceneImporter() noexcept = default;

	LevelSceneImporter(const LevelSceneImporter&) = delete;
	LevelSceneImporter& operator=(const LevelSceneImporter&) = delete;
	LevelSceneImporter(LevelSceneImporter&&) = delete;
	LevelSceneImporter& operator=(LevelSceneImporter&&) = delete;

  private:
	static ImportedSceneData BuildImportedSceneData(SceneImportResult&& importResult);

	void ImportActiveLevelIfPresent() noexcept;
	void HandleLevelChanged(const LevelChangedEventArgs& args) noexcept;
	bool ImportLevel(const LevelAsset& level) noexcept;
	bool ImportMeshRequest(const ImportedMeshRequest& request) noexcept;

	LevelManager* m_levelManager = nullptr;
	GameScene* m_gameScene = nullptr;
	ScopedEventHandle m_levelChangedHandle;
};