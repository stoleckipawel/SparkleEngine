#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Scene/Camera/CameraDesc.h"
#include "GameFramework/Public/Scene/Lighting/SceneLightDesc.h"
#include "GameFramework/Public/Scene/Materials/MaterialVariant.h"
#include "GameFramework/Public/Scene/Transform.h"
#include "GameFramework/Public/World/EntityId.h"
#include "GameFramework/Public/World/SkyEnvironment.h"

#include <cstdint>
#include <optional>
#include <string>
#include <variant>

struct SetActiveCameraCommand final
{
	EntityId Entity;
};

struct SetLocalTransformCommand final
{
	EntityId Entity;
	Transform Value;
};

struct SetCameraDescriptionCommand final
{
	EntityId Entity;
	CameraDesc Value;
};

struct SetEntityVisibilityCommand final
{
	EntityId Entity;
	bool Value = true;
};

struct SetLightDescriptionCommand final
{
	EntityId Entity;
	SceneLightDesc Value;
};

struct SetSkyEnvironmentCommand final
{
	std::optional<SkyEnvironment> Value;
};

struct SetMaterialVariantCommand final
{
	MaterialVariantIndex Value = 0;
};

using WorldEditPayload = std::variant<
    SetActiveCameraCommand,
    SetLocalTransformCommand,
    SetCameraDescriptionCommand,
    SetEntityVisibilityCommand,
    SetLightDescriptionCommand,
    SetSkyEnvironmentCommand,
    SetMaterialVariantCommand>;

struct WorldEditCommand final
{
	std::uint64_t RequestId = 0;
	WorldEditPayload Payload;
};

enum class WorldEditResultStatus : std::uint8_t
{
	Accepted,
	Stale,
	Rejected
};

struct SPARKLE_ENGINE_API WorldEditResult final
{
	std::uint64_t RequestId = 0;
	WorldEditResultStatus Status = WorldEditResultStatus::Rejected;
	std::string Message;

	bool IsAccepted() const noexcept { return Status == WorldEditResultStatus::Accepted; }
};
