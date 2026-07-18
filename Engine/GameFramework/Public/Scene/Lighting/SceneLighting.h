#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Scene/Lighting/LightingSnapshot.h"
#include "GameFramework/Public/Scene/Lighting/SceneLightDesc.h"
#include "GameFramework/Public/World/EntityId.h"

#include <cstddef>
#include <optional>
#include <vector>

class GameWorld;

class SPARKLE_ENGINE_API SceneLighting final
{
  public:
	explicit SceneLighting(GameWorld& world) noexcept;
	~SceneLighting() noexcept = default;

	SceneLighting(const SceneLighting&) = delete;
	SceneLighting& operator=(const SceneLighting&) = delete;
	SceneLighting(SceneLighting&&) = delete;
	SceneLighting& operator=(SceneLighting&&) = delete;

	std::size_t GetLightCount() const noexcept;
	EntityId GetLightEntity(std::size_t index) const noexcept;
	std::optional<SceneLightDesc> GetLight(std::size_t index) const;
	std::optional<SceneLightDesc> GetLight(EntityId entity) const;
	bool IsLightVisible(std::size_t index) const noexcept;
	bool IsLightVisible(EntityId entity) const noexcept;
	void SetLightVisible(std::size_t index, bool visible);
	void SetLightVisible(EntityId entity, bool visible);
	bool SetLight(std::size_t lightIndex, SceneLightDesc light);
	bool SetLight(EntityId entity, SceneLightDesc light);

	void ApplyFromDesc(const std::vector<SceneLightDesc>& lights);
	void AppendLight(SceneLightDesc light);
	std::vector<SceneLightDesc> CaptureToDesc() const;
	LightingSnapshot CaptureSnapshot() const;

  private:
	friend class GameWorld;
	GameWorld* m_world = nullptr;
};
