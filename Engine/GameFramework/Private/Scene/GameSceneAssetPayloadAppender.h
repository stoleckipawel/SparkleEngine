#pragma once

struct SceneAssetPayload;

class SceneCameras;
class SceneLighting;
class SceneMaterials;
class SceneMaterialVariants;
class SceneMeshes;
class SceneSkeletons;
class SceneAnimations;
class SceneTextures;

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
	    SceneAnimations& animations,
	    SceneTextures& textures) noexcept;

	bool Append(SceneAssetPayload&& sceneAssetPayload);

  private:
	SceneCameras& m_cameras;
	SceneLighting& m_lighting;
	SceneMaterials& m_materials;
	SceneMaterialVariants& m_materialVariants;
	SceneMeshes& m_meshes;
	SceneSkeletons& m_skeletons;
	SceneAnimations& m_animations;
	SceneTextures& m_textures;
};
