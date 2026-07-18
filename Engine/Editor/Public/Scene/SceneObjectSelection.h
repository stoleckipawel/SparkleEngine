#pragma once

#include <cstdint>
#include "World/EntityId.h"

enum class SceneObjectType : std::uint8_t
{
	None = 0,
	Camera,
	Sky,
	Light,
	Mesh
};

struct SceneObjectSelection final
{
	SceneObjectType type = SceneObjectType::None;
	EntityId entity;

	static SceneObjectSelection None() noexcept { return {}; }
	static SceneObjectSelection Camera(EntityId entity) noexcept { return {SceneObjectType::Camera, entity}; }
	static SceneObjectSelection Sky() noexcept { return {SceneObjectType::Sky, EntityId::Invalid()}; }
	static SceneObjectSelection Light(EntityId entity) noexcept { return {SceneObjectType::Light, entity}; }
	static SceneObjectSelection Mesh(EntityId entity) noexcept { return {SceneObjectType::Mesh, entity}; }

	bool IsNone() const noexcept { return type == SceneObjectType::None; }
	bool operator==(const SceneObjectSelection& other) const noexcept { return type == other.type && entity == other.entity; }
};
