#pragma once

#include "GameFramework/Public/Scene/Camera/CameraSnapshot.h"
#include "GameFramework/Public/Scene/Animations/AnimationOutput.h"
#include "GameFramework/Public/Scene/Lighting/LightingSnapshot.h"
#include "GameFramework/Public/Scene/Materials/MaterialSnapshot.h"
#include "GameFramework/Public/Scene/Meshes/MeshSnapshot.h"
#include "GameFramework/Public/Scene/Sky/SceneSkySnapshot.h"
#include "GameFramework/Public/Scene/Textures/TextureSnapshot.h"

struct GameWorldSnapshot;

struct RenderSceneSnapshot final
{
	CameraSnapshot camera = {};
	AnimationOutput animations = {};
	LightingSnapshot lighting = {};
	SceneSkySnapshot sky = {};
	TextureSnapshot textures = {};
	MeshSnapshot meshes = {};
	MaterialSnapshot materials = {};

	void Capture(GameWorldSnapshot&& gameWorldSnapshot) noexcept;
	void Reset() noexcept;
};
