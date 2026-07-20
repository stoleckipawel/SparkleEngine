#pragma once

#include "GameFramework/Public/Assets/Cooked/CookedAssetCommon.h"

#include <cstdint>
#include <string>

namespace ECS
{
	enum class AuthoredObjectKind : std::uint8_t
	{
		Unknown = 0,
		Camera,
		Light,
		MeshInstance,
		Animation
	};

	struct AuthoredIdentity final
	{
		Assets::CookedAssetId SourceAssetId = Assets::InvalidCookedAssetId;
		std::uint64_t SourceInstanceId = 0;
		std::uint64_t SourceObjectId = 0;
		AuthoredObjectKind Kind = AuthoredObjectKind::Unknown;
	};

	struct Name final
	{
		std::string Value;
	};

	struct EditorMetadata final
	{
		bool Selectable = true;
		bool Locked = false;
		bool HiddenInHierarchy = false;
	};
}
