#pragma once

#include "Scene/Camera/CameraSnapshot.h"
#include "Scene/Lighting/LightingSnapshot.h"
#include "Scene/Materials/MaterialSnapshot.h"
#include "Scene/Meshes/MeshSnapshot.h"
#include "Scene/Textures/TextureSnapshot.h"

class GameScene;

struct RenderSceneSnapshot
{
	CameraSnapshot camera = {};
	LightingSnapshot lighting = {};
	TextureSnapshot textures = {};
	MeshSnapshot meshes = {};
	MaterialSnapshot materials = {};

	void Capture(const GameScene& gameScene);
	void Reset() noexcept
	{
		camera = {};
		lighting = {};
		textures.Reset();
		meshes.Reset();
		materials.Reset();
	}
};
