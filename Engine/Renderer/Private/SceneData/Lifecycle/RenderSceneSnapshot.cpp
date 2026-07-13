#include "PCH.h"

#include "RenderSceneSnapshot.h"

#include "GameFramework/Public/Scene/GameSceneSnapshot.h"

#include <utility>

void RenderSceneSnapshot::Capture(GameSceneSnapshot&& gameSceneSnapshot) noexcept
{
	camera = std::move(gameSceneSnapshot.camera);
	animations = std::move(gameSceneSnapshot.animations);
	lighting = std::move(gameSceneSnapshot.lighting);
	sky = std::move(gameSceneSnapshot.sky);
	textures = std::move(gameSceneSnapshot.textures);
	meshes = std::move(gameSceneSnapshot.meshes);
	materials = std::move(gameSceneSnapshot.materials);
}

void RenderSceneSnapshot::Reset() noexcept
{
	camera = {};
	animations.Reset();
	lighting = {};
	sky = {};
	textures.Reset();
	meshes.Reset();
	materials.Reset();
}
