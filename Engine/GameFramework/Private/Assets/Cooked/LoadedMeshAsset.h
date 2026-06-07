#pragma once

#include "Assets/Cooked/CookedMeshAsset.h"
#include "Scene/Meshes/SkeletalMeshData.h"
#include "Scene/Meshes/StaticMeshData.h"

#include <variant>

namespace Assets
{
	struct LoadedMeshAsset
	{
		using Payload = std::variant<StaticMeshData, SkeletalMeshData>;

		Payload payload;

		CookedMeshAssetKind GetAssetKind() const noexcept
		{
			return std::holds_alternative<SkeletalMeshData>(payload) ? CookedMeshAssetKind::Skeletal : CookedMeshAssetKind::Static;
		}
		bool IsStatic() const noexcept { return std::holds_alternative<StaticMeshData>(payload); }
		bool IsSkeletal() const noexcept { return std::holds_alternative<SkeletalMeshData>(payload); }
		StaticMeshData& AsStatic() noexcept { return std::get<StaticMeshData>(payload); }
		SkeletalMeshData& AsSkeletal() noexcept { return std::get<SkeletalMeshData>(payload); }
		const StaticMeshData& AsStatic() const noexcept { return std::get<StaticMeshData>(payload); }
		const SkeletalMeshData& AsSkeletal() const noexcept { return std::get<SkeletalMeshData>(payload); }
	};
}
