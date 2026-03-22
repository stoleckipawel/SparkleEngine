#pragma once

#include <cstddef>
#include <cstdint>

enum class SceneObjectType : std::uint8_t
{
	None = 0,
	Camera,
	DirectionalLight,
	Mesh
};

struct SceneObjectSelection final
{
	SceneObjectType type = SceneObjectType::None;
	std::size_t index = 0;

	static SceneObjectSelection None() noexcept { return {}; }
	static SceneObjectSelection Camera() noexcept { return {SceneObjectType::Camera, 0}; }
	static SceneObjectSelection DirectionalLight(std::size_t lightIndex) noexcept { return {SceneObjectType::DirectionalLight, lightIndex}; }
	static SceneObjectSelection Mesh(std::size_t meshIndex) noexcept { return {SceneObjectType::Mesh, meshIndex}; }

	bool IsNone() const noexcept { return type == SceneObjectType::None; }
	bool operator==(const SceneObjectSelection& other) const noexcept
	{
		return type == other.type && index == other.index;
	}
};