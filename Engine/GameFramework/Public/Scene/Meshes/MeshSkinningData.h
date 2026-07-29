#pragma once

#include <cstdint>
#include <type_traits>

struct VertexSkinInfluence
{
	std::uint16_t jointIndices[8] = {};
	float jointWeights[8] = {};
};

static_assert(std::is_trivially_copyable_v<VertexSkinInfluence>, "VertexSkinInfluence must be trivially copyable for GPU upload");
