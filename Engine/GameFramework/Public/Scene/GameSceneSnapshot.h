#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Scene/Camera/CameraSnapshot.h"
#include "GameFramework/Public/Scene/Lighting/LightingSnapshot.h"
#include "GameFramework/Public/Scene/Materials/MaterialSnapshot.h"
#include "GameFramework/Public/Scene/Meshes/MeshSnapshot.h"
#include "GameFramework/Public/Scene/Textures/TextureSnapshot.h"

struct SPARKLE_ENGINE_API GameSceneSnapshot
{
	CameraSnapshot camera = {};
	LightingSnapshot lighting = {};
	TextureSnapshot textures = {};
	MeshSnapshot meshes = {};
	MaterialSnapshot materials = {};

	void Reset() noexcept
	{
		camera = {};
		lighting = {};
		textures.Reset();
		meshes.Reset();
		materials.Reset();
	}
};