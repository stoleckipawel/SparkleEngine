#pragma once

#include "GameFramework/Public/Scene/Camera/CameraSnapshot.h"
#include "GameFramework/Public/Scene/Lighting/LightingSnapshot.h"
#include "GameFramework/Public/Scene/Materials/MaterialSnapshot.h"
#include "GameFramework/Public/Scene/Meshes/MeshSnapshot.h"
#include "GameFramework/Public/Scene/Textures/TextureSnapshot.h"

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
