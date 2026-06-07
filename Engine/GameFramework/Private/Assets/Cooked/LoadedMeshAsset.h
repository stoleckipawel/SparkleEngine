#pragma once

#include "Assets/Cooked/CookedMeshAsset.h"
#include "Scene/Meshes/MeshSkinningData.h"

#include <variant>

namespace Assets
{
	struct LoadedMeshAsset
	{
		using Payload = std::variant<MeshData, SkeletalMeshData>;

		Payload payload;

		CookedMeshAssetKind GetAssetKind() const noexcept
		{
			return std::holds_alternative<SkeletalMeshData>(payload) ? CookedMeshAssetKind::Skeletal : CookedMeshAssetKind::Static;
		}
		bool IsStatic() const noexcept { return std::holds_alternative<MeshData>(payload); }
		bool IsSkeletal() const noexcept { return std::holds_alternative<SkeletalMeshData>(payload); }
		MeshData& AsStatic() noexcept { return std::get<MeshData>(payload); }
		SkeletalMeshData& AsSkeletal() noexcept { return std::get<SkeletalMeshData>(payload); }
		const MeshData& AsStatic() const noexcept { return std::get<MeshData>(payload); }
		const SkeletalMeshData& AsSkeletal() const noexcept { return std::get<SkeletalMeshData>(payload); }
	};
}
