#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Scene/Sky/SceneSkyDesc.h"
#include "GameFramework/Public/Scene/Sky/SceneSkySnapshot.h"

#include <optional>

class GameWorld;

class SPARKLE_ENGINE_API SceneSky final
{
  public:
	explicit SceneSky(GameWorld& world) noexcept : m_world(&world) {}
	void ApplyFromDesc(std::optional<SceneSkyDesc> sky) noexcept;
	void Reset() noexcept;

	bool HasSky() const noexcept;
	std::optional<SceneSkyDesc> GetSky() const;
	void SetSky(SceneSkyDesc sky = {});
	void RemoveSky() noexcept;

	std::optional<SceneSkyDesc> CaptureToDesc() const;
	SceneSkySnapshot CaptureSnapshot() const;

  private:
	GameWorld* m_world = nullptr;
};
