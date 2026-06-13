#pragma once

#include "GameFramework/Public/Scene/Camera/CameraSnapshot.h"
#include "GameFramework/Public/Scene/Animations/SceneAnimation.h"
#include "GameFramework/Public/Scene/Lighting/LightingSnapshot.h"
#include "GameFramework/Public/Scene/Materials/MaterialSnapshot.h"
#include "GameFramework/Public/Scene/Meshes/MeshSnapshot.h"
#include "GameFramework/Public/Scene/Textures/TextureSnapshot.h"

struct GameSceneSnapshot;

struct RenderSceneSnapshot final
{
	CameraSnapshot camera = {};
	SceneAnimationSnapshot animations = {};
	LightingSnapshot lighting = {};
	TextureSnapshot textures = {};
	MeshSnapshot meshes = {};
	MaterialSnapshot materials = {};

	void Capture(GameSceneSnapshot&& gameSceneSnapshot) noexcept;
	void Reset() noexcept;
};
