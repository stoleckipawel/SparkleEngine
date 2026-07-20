#pragma once

#include "World/Resources/AnimationClipResourceStore.h"
#include "World/Resources/MaterialResourceStore.h"
#include "World/Resources/MaterialVariantResourceStore.h"
#include "World/Resources/SkeletonResourceStore.h"
#include "World/Resources/TextureResourceStore.h"

#include <cstdint>

struct GameWorldResourceStores final
{
	GameWorldResourceStores() noexcept;
	std::uint32_t Generation = 0;
	ECS::AnimationClipResourceStore AnimationClips;
	MaterialResourceStore Materials;
	MaterialVariantResourceStore MaterialVariants;
	SkeletonResourceStore Skeletons;
	TextureResourceStore Textures;
};
