#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Scene/Sky/SceneSkyDesc.h"
#include "GameFramework/Public/Scene/Sky/SceneSkySnapshot.h"

#include <optional>

class SPARKLE_ENGINE_API SceneSky final
{
  public:
	void ApplyFromDesc(std::optional<SceneSkyDesc> sky) noexcept;
	void Reset() noexcept;

	bool HasSky() const noexcept { return m_sky.has_value(); }
	const SceneSkyDesc* GetSky() const noexcept;
	SceneSkyDesc* GetSky() noexcept;
	void SetSky(SceneSkyDesc sky = {});
	void RemoveSky() noexcept;

	std::optional<SceneSkyDesc> CaptureToDesc() const;
	SceneSkySnapshot CaptureSnapshot() const;

  private:
	std::optional<SceneSkyDesc> m_sky;
};
