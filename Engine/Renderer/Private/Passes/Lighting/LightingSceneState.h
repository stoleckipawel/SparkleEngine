#pragma once

#include <cstdint>

struct PreparedRenderScene;

struct LightingSceneStateIdentity final
{
	std::uint64_t Geometry = 0u;
	std::uint64_t Deformation = 0u;
	std::uint64_t Materials = 0u;
	std::uint64_t Lights = 0u;
	std::uint64_t Environment = 0u;
};

LightingSceneStateIdentity BuildLightingSceneStateIdentity(const PreparedRenderScene& scene) noexcept;
std::uint64_t BuildLightingSceneInvalidationHash(const PreparedRenderScene& scene) noexcept;
