#include "PCH.h"

#include "RenderSceneSnapshot.h"

#include "GameFramework/Public/World/GameWorldSnapshot.h"

#include <utility>

void RenderSceneSnapshot::Capture(GameWorldSnapshot&& gameWorldSnapshot) noexcept
{
	camera = std::move(gameWorldSnapshot.camera);
	animations = std::move(gameWorldSnapshot.animations);
	lighting = std::move(gameWorldSnapshot.lighting);
	sky = std::move(gameWorldSnapshot.sky);
	textures = std::move(gameWorldSnapshot.textures);
	meshes = std::move(gameWorldSnapshot.meshes);
	materials = std::move(gameWorldSnapshot.materials);
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
