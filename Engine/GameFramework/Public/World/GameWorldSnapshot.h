#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Scene/Camera/CameraSnapshot.h"
#include "GameFramework/Public/Scene/Animations/SceneAnimation.h"
#include "GameFramework/Public/Scene/Lighting/LightingSnapshot.h"
#include "GameFramework/Public/Scene/Materials/MaterialSnapshot.h"
#include "GameFramework/Public/Scene/Meshes/MeshSnapshot.h"
#include "GameFramework/Public/Scene/Sky/SceneSkySnapshot.h"
#include "GameFramework/Public/Scene/Textures/TextureSnapshot.h"

struct SPARKLE_ENGINE_API GameWorldSnapshot
{
	CameraSnapshot camera = {};
	SceneAnimationSnapshot animations = {};
	LightingSnapshot lighting = {};
	SceneSkySnapshot sky = {};
	TextureSnapshot textures = {};
	MeshSnapshot meshes = {};
	MaterialSnapshot materials = {};

	void Reset() noexcept
	{
		camera = {};
		animations.Reset();
		lighting = {};
		sky = {};
		textures.Reset();
		meshes.Reset();
		materials.Reset();
	}
};
