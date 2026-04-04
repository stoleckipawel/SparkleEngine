#include "PCH.h"

#include "SceneData/RenderSceneSnapshot.h"

#include "Scene/GameScene.h"
#include "Scene/Meshes/Mesh.h"

void RenderSceneSnapshot::Capture(const GameScene& gameScene)
{
	Reset();
	camera = gameScene.GetSceneCamera().CaptureSnapshot();
	lighting = gameScene.GetLighting().CaptureSnapshot();
	textures = gameScene.GetTextures().CaptureSnapshot();
	materials = gameScene.GetMaterials().CaptureSnapshot();
	meshes = gameScene.GetMeshes().CaptureSnapshot();
}