#pragma once

#include <memory>

class EngineRenderingSettingsSection;
class RenderingSettingsPanel;

class SettingsPanel final
{
  public:
	SettingsPanel();
	~SettingsPanel();

	void SetOpen(bool open) noexcept;
	bool IsOpen() const noexcept { return m_isOpen; }
	void SetRenderingSettings(EngineRenderingSettingsSection* renderingSettings) noexcept;
	void BuildUI(bool disableInteraction);

  private:
	enum class Section
	{
		Rendering,
	};

	void DrawNavigation();

	EngineRenderingSettingsSection* m_renderingSettings = nullptr;
	std::unique_ptr<RenderingSettingsPanel> m_renderingPanel;
	Section m_activeSection = Section::Rendering;
	bool m_isOpen = false;
	bool m_refreshFromRuntimeOnNextOpen = true;
};
