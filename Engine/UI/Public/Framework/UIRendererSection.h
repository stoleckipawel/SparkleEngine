#pragma once

#include <cstdint>

enum class UIRendererSectionId : std::uint8_t
{
	Stats = 0,
	ViewMode,
	Time,
	Scene,
	Camera,
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
