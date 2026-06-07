#pragma once

#include <cstdint>
#include <type_traits>

struct VertexSkinInfluence
{
	std::uint16_t jointIndices[4] = {0, 0, 0, 0};
	float jointWeights[4] = {0.0f, 0.0f, 0.0f, 0.0f};
};

static_assert(std::is_trivially_copyable_v<VertexSkinInfluence>, "VertexSkinInfluence must be trivially copyable for GPU upload");
