#pragma once

#include "GameFramework/Public/Assets/Cooked/CookedAssetCommon.h"
#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Scene/Camera/CameraDesc.h"
#include "GameFramework/Public/Scene/Camera/CameraMovementSettings.h"
#include "GameFramework/Public/Scene/Lighting/SceneLightDesc.h"
#include "GameFramework/Public/Scene/Materials/MaterialHandle.h"
#include "GameFramework/Public/Scene/Meshes/SceneMeshKind.h"
#include "GameFramework/Public/Scene/Transform.h"
#include "GameFramework/Public/World/EntityId.h"
#include "GameFramework/Public/World/SkyEnvironment.h"
#include "GameFramework/Public/World/WorldChange.h"

#include <DirectXMath.h>
#include <memory>
#include <limits>
#include <optional>
#include <span>
#include <string>

namespace ECS
{
	class GameWorldState;
}

struct WorldCameraReadData final
{
	EntityId Entity;
	std::string Name;
	CameraDesc Description;
	CameraMovementSettings Movement;
	Transform LocalTransform;
	DirectX::XMFLOAT4X4 WorldMatrix{};
	DirectX::XMFLOAT3 Direction{0.0f, 0.0f, 1.0f};
	float AspectRatio = 1.0f;
	bool Visible = true;
	bool Active = false;
};

struct WorldLightReadData final
{
	EntityId Entity;
	SceneLightDesc Description;
};

struct WorldMeshReadData final
{
	EntityId Entity;
	Transform LocalTransform;
	DirectX::XMFLOAT4X4 WorldMatrix{};
	MaterialHandle Material = MaterialHandle::Invalid();
	Assets::CookedAssetId MeshAssetId = Assets::InvalidCookedAssetId;
	Assets::CookedAssetId SkeletonAssetId = Assets::InvalidCookedAssetId;
	SceneMeshKind Kind = SceneMeshKind::Static;
	std::uint32_t SourceNodeIndex = (std::numeric_limits<std::uint32_t>::max)();
	bool Visible = true;
};

class SPARKLE_ENGINE_API WorldReadView final
{
  public:
	WorldReadView() noexcept = default;
	bool IsValid() const noexcept { return m_storage != nullptr; }
	std::uint64_t GetGeneration() const noexcept;
	WorldSequence GetSequence() const noexcept;
	std::span<const WorldCameraReadData> GetCameras() const noexcept;
	std::span<const WorldLightReadData> GetLights() const noexcept;
	std::span<const WorldMeshReadData> GetMeshes() const noexcept;
	const std::optional<SkyEnvironment>& GetSkyEnvironment() const noexcept;

  private:
	struct Storage;
	friend class ECS::GameWorldState;
	explicit WorldReadView(std::shared_ptr<const Storage> storage) noexcept : m_storage(std::move(storage)) {}
	std::shared_ptr<const Storage> m_storage;
};
