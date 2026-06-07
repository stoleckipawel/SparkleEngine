#pragma once

struct SceneAssetPayload;

class SceneCameras;
class SceneLighting;
class SceneMaterials;
class SceneMeshes;
class SceneTextures;

class GameSceneAssetPayloadAppender final
{
  public:
	GameSceneAssetPayloadAppender(
	    SceneCameras& cameras,
	    SceneLighting& lighting,
	    SceneMaterials& materials,
	    SceneMeshes& meshes,
	    SceneTextures& textures) noexcept;

	bool Append(SceneAssetPayload&& sceneAssetPayload);

  private:
	SceneCameras& m_cameras;
	SceneLighting& m_lighting;
	SceneMaterials& m_materials;
	SceneMeshes& m_meshes;
	SceneTextures& m_textures;
};
