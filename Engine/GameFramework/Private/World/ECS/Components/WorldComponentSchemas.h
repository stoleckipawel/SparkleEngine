#pragma once

#include "World/ECS/ComponentSchema.h"
#include "World/ECS/Components/AnimationComponents.h"
#include "World/ECS/Components/EditorComponents.h"
#include "World/ECS/Components/RenderingComponents.h"
#include "World/ECS/Components/TransformComponents.h"

namespace ECS
{
	template <> struct ComponentSchemaTraits<LocalTransform> final
	{
		static constexpr ComponentSchema Schema{MakeComponentSchemaId("sparkle.world.LocalTransform"), 1, "sparkle.world.LocalTransform"};
	};
	template <> struct ComponentSchemaTraits<WorldTransform> final
	{
		static constexpr ComponentSchema Schema{MakeComponentSchemaId("sparkle.world.WorldTransform"), 1, "sparkle.world.WorldTransform"};
	};
	template <> struct ComponentSchemaTraits<CameraDerivedState> final
	{
		static constexpr ComponentSchema Schema{
		    MakeComponentSchemaId("sparkle.world.CameraDerivedState"), 1, "sparkle.world.CameraDerivedState"};
	};
	template <> struct ComponentSchemaTraits<MeshInstance> final
	{
		static constexpr ComponentSchema Schema{MakeComponentSchemaId("sparkle.world.MeshInstance"), 1, "sparkle.world.MeshInstance"};
	};
	template <> struct ComponentSchemaTraits<Visibility> final
	{
		static constexpr ComponentSchema Schema{MakeComponentSchemaId("sparkle.world.Visibility"), 1, "sparkle.world.Visibility"};
	};
	template <> struct ComponentSchemaTraits<Camera> final
	{
		static constexpr ComponentSchema Schema{MakeComponentSchemaId("sparkle.world.Camera"), 1, "sparkle.world.Camera"};
	};
	template <> struct ComponentSchemaTraits<CameraMovement> final
	{
		static constexpr ComponentSchema Schema{MakeComponentSchemaId("sparkle.world.CameraMovement"), 1, "sparkle.world.CameraMovement"};
	};
	template <> struct ComponentSchemaTraits<Light> final
	{
		static constexpr ComponentSchema Schema{MakeComponentSchemaId("sparkle.world.Light"), 1, "sparkle.world.Light"};
	};
	template <> struct ComponentSchemaTraits<AnimationState> final
	{
		static constexpr ComponentSchema Schema{MakeComponentSchemaId("sparkle.world.AnimationState"), 1, "sparkle.world.AnimationState"};
	};
	template <> struct ComponentSchemaTraits<MorphState> final
	{
		static constexpr ComponentSchema Schema{MakeComponentSchemaId("sparkle.world.MorphState"), 1, "sparkle.world.MorphState"};
	};
	template <> struct ComponentSchemaTraits<SkinningState> final
	{
		static constexpr ComponentSchema Schema{MakeComponentSchemaId("sparkle.world.SkinningState"), 1, "sparkle.world.SkinningState"};
	};
	template <> struct ComponentSchemaTraits<Name> final
	{
		static constexpr ComponentSchema Schema{MakeComponentSchemaId("sparkle.world.Name"), 1, "sparkle.world.Name"};
	};
	template <> struct ComponentSchemaTraits<AuthoredIdentity> final
	{
		static constexpr ComponentSchema Schema{MakeComponentSchemaId("sparkle.world.AuthoredIdentity"), 2, "sparkle.world.AuthoredIdentity"};
	};
	template <> struct ComponentSchemaTraits<EditorMetadata> final
	{
		static constexpr ComponentSchema Schema{MakeComponentSchemaId("sparkle.world.EditorMetadata"), 2, "sparkle.world.EditorMetadata"};
	};

	static_assert(GetComponentSchema<LocalTransform>().Id.Value == 0x189d4521623a0032ull);
	static_assert(GetComponentSchema<WorldTransform>().Id.Value == 0xda6d95badcb7e697ull);
	static_assert(GetComponentSchema<CameraDerivedState>().Id.Value == 0x3e943bc738fe2b5eull);
	static_assert(GetComponentSchema<MeshInstance>().Id.Value == 0x75f1f38b60ad26c9ull);
	static_assert(GetComponentSchema<Visibility>().Id.Value == 0xe0676cf454dca3dfull);
	static_assert(GetComponentSchema<Camera>().Id.Value == 0x75e15d3fe7ce766cull);
	static_assert(GetComponentSchema<CameraMovement>().Id.Value == 0xe2ff42e4c5c21181ull);
	static_assert(GetComponentSchema<Light>().Id.Value == 0x866dcadfe35d0545ull);
	static_assert(GetComponentSchema<AnimationState>().Id.Value == 0x16b8b614ff6ca6b4ull);
	static_assert(GetComponentSchema<MorphState>().Id.Value == 0xf5aa903283211a80ull);
	static_assert(GetComponentSchema<SkinningState>().Id.Value == 0x44392a61286e7591ull);
	static_assert(GetComponentSchema<Name>().Id.Value == 0xc148fc459018a120ull);
	static_assert(GetComponentSchema<AuthoredIdentity>().Id.Value == 0xd39548d1c7e5b7ebull);
	static_assert(GetComponentSchema<EditorMetadata>().Id.Value == 0xfecdcc244248992bull);
}
