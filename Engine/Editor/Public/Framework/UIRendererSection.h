#pragma once

#include <cstdint>

enum class UIRendererSectionId : std::uint8_t
{
	ViewMode = 0,
	Time,
	Scene,
	Camera,
	Lighting,
	Count
};

class UIRendererSection
{
  public:
	virtual ~UIRendererSection() = default;

	virtual UIRendererSectionId GetId() const noexcept = 0;

	virtual const char* GetTitle() const noexcept = 0;

	virtual void BuildUI() = 0;
};
