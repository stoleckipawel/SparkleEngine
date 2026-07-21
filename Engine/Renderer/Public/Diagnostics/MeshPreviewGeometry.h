#pragma once

#include <cstdint>
#include <vector>

struct MeshPreviewVertex final
{
	float X = 0.0f;
	float Y = 0.0f;
	float Z = 0.0f;
};

// Immutable diagnostic value copied from a renderer-owned mesh resource.
struct MeshPreviewGeometry final
{
	std::vector<MeshPreviewVertex> Vertices;
	std::vector<std::uint32_t> Indices;

	bool IsValid() const noexcept { return !Vertices.empty() && !Indices.empty(); }
};
