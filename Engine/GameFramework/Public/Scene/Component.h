#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"

class SPARKLE_ENGINE_API Component
{
  public:
	virtual ~Component();

	bool IsVisible() const noexcept { return m_visible; }
	void SetVisible(bool visible) noexcept { m_visible = visible; }

  private:
	bool m_visible = true;
};
