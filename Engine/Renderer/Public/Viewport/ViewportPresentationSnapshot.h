#pragma once

#include "ViewportContracts.h"
#include "Renderer/Public/UI/UiTextureHandle.h"

#include <cstdint>

struct SPARKLE_RENDERER_API ViewportPresentationSnapshot final
{
	ViewportRenderProducts Products;
	UiTextureHandle Texture;
	std::uint64_t PublicationSequence = 0;
};

static_assert(sizeof(ViewportPresentationSnapshot) <= 2048);
