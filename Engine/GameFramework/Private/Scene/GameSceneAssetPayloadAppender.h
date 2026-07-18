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
	class SceneWorld;
}

class GameSceneAssetPayloadAppender final
{
  public:
	GameSceneAssetPayloadAppender(
	    SceneCameras& cameras,
	    SceneLighting& lighting,
	    SceneMaterials& materials,
	    SceneMaterialVariants& materialVariants,
	    SceneMeshes& meshes,
	    SceneSkeletons& skeletons,
	    ECS::SceneWorld& world,
	    SceneTextures& textures) noexcept;

	bool Append(SceneAssetPayload&& sceneAssetPayload);

  private:
	SceneCameras& m_cameras;
	SceneLighting& m_lighting;
	SceneMaterials& m_materials;
	SceneMaterialVariants& m_materialVariants;
	SceneMeshes& m_meshes;
	SceneSkeletons& m_skeletons;
	ECS::SceneWorld& m_world;
	SceneTextures& m_textures;
};
