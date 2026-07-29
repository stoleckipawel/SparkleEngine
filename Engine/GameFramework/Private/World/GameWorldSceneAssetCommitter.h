#pragma once

struct SceneAssetPayload;
struct GameWorldResourceStores;

namespace ECS
{
	class GameWorldState;
}

class GameWorldSceneAssetCommitter final
{
  public:
	GameWorldSceneAssetCommitter(ECS::GameWorldState& world, GameWorldResourceStores& resources) noexcept;

	void Commit(SceneAssetPayload&& sceneAssetPayload);

  private:
	ECS::GameWorldState& m_state;
	GameWorldResourceStores& m_resources;
};
