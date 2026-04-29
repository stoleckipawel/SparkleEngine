#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"

class Entity;

class SPARKLE_ENGINE_API Component
{
  protected:
	Entity* owner = nullptr;

  public:
	virtual ~Component();

	virtual void Initialize();
	virtual void Update(float deltaTime);
	virtual void Render();

	void SetOwner(Entity* entity);
	Entity* GetOwner() const;

	bool IsVisible() const noexcept { return m_visible; }
	void SetVisible(bool visible) noexcept { m_visible = visible; }

  private:
	bool m_visible = true;
};