#pragma once

#include "Framework/UIRendererSection.h"

#include <string>

class LevelManager;

class CameraSection final : public UIRendererSection
{
  public:
	explicit CameraSection(LevelManager& levelManager) noexcept;
	~CameraSection() = default;

	CameraSection(const CameraSection&) = delete;
	CameraSection(CameraSection&&) = delete;
	CameraSection& operator=(const CameraSection&) = delete;
	CameraSection& operator=(CameraSection&&) = delete;

	UIRendererSectionId GetId() const noexcept override { return UIRendererSectionId::Camera; }
	const char* GetTitle() const noexcept override { return "Camera"; }

	void BuildUI() override;

  private:
	LevelManager* m_levelManager = nullptr;
	std::string m_statusMessage;
	bool m_bLastSaveSucceeded = false;
};