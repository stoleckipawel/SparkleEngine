#include "PCH.h"

#include "RenderSceneSnapshot.h"

#include <utility>

void RenderSceneSnapshot::Capture(GameSceneSnapshot&& gameSceneSnapshot) noexcept
{
	static_cast<GameSceneSnapshot&>(*this) = std::move(gameSceneSnapshot);
}