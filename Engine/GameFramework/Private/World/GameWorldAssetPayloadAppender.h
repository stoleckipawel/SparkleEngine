#pragma once

struct SceneAssetPayload;

class SceneCameras;
class SceneLighting;
class SceneMaterials;
class SceneMaterialVariants;
class SceneMeshes;
class SceneSkeletons;
class SceneTextures;

namespace ECS
{
	class GameWorldState;
}

class GameWorldAssetPayloadAppender final
{
  public:
	GameWorldAssetPayloadAppender(
	    SceneCameras& cameras,
	    SceneLighting& lighting,
	    SceneMaterials& materials,
	    SceneMaterialVariants& materialVariants,
	    SceneMeshes& meshes,
	    SceneSkeletons& skeletons,
	    ECS::GameWorldState& world,
	    SceneTextures& textures) noexcept;

	bool Append(SceneAssetPayload&& sceneAssetPayload);

  private:
	SceneCameras& m_cameras;
	SceneLighting& m_lighting;
	SceneMaterials& m_materials;
	SceneMaterialVariants& m_materialVariants;
	SceneMeshes& m_meshes;
	SceneSkeletons& m_skeletons;
	ECS::GameWorldState& m_state;
	SceneTextures& m_textures;
};
