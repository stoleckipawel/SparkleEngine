#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Scene/Lighting/PointLightDesc.h"
#include "GameFramework/Public/Scene/Lighting/SceneDirectionalLightDesc.h"
#include "GameFramework/Public/Scene/Lighting/SceneLightCommonDesc.h"
#include "GameFramework/Public/Scene/Lighting/SceneLightKind.h"
#include "GameFramework/Public/Scene/Lighting/SpotLightDesc.h"

#include <variant>

using SceneLightPayload = std::variant<std::monostate, SceneDirectionalLightDesc, PointLightDesc, SpotLightDesc>;

struct SPARKLE_ENGINE_API SceneLightDesc
{
	SceneLightCommonDesc common;
	SceneLightPayload payload;

	SceneLightKind GetKind() const noexcept
	{
		if (std::holds_alternative<SceneDirectionalLightDesc>(payload))
		{
			return SceneLightKind::Directional;
		}

		if (std::holds_alternative<PointLightDesc>(payload))
		{
			return SceneLightKind::Point;
		}

		if (std::holds_alternative<SpotLightDesc>(payload))
		{
			return SceneLightKind::Spot;
		}

		return SceneLightKind::Unknown;
	}

	const SceneDirectionalLightDesc* GetDirectional() const noexcept { return std::get_if<SceneDirectionalLightDesc>(&payload); }
	SceneDirectionalLightDesc* GetDirectional() noexcept { return std::get_if<SceneDirectionalLightDesc>(&payload); }
	const PointLightDesc* GetPoint() const noexcept { return std::get_if<PointLightDesc>(&payload); }
	PointLightDesc* GetPoint() noexcept { return std::get_if<PointLightDesc>(&payload); }
	const SpotLightDesc* GetSpot() const noexcept { return std::get_if<SpotLightDesc>(&payload); }
	SpotLightDesc* GetSpot() noexcept { return std::get_if<SpotLightDesc>(&payload); }

	bool IsDirectional() const noexcept { return GetDirectional() != nullptr; }
	bool IsPoint() const noexcept { return GetPoint() != nullptr; }
	bool IsSpot() const noexcept { return GetSpot() != nullptr; }
};
