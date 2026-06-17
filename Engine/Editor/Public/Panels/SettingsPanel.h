#pragma once

class EditorRenderingSettingsSection;

class SettingsPanel final
{
  public:
	void SetOpen(bool open) noexcept;
	bool IsOpen() const noexcept { return m_isOpen; }
	void SetRenderingSettings(EditorRenderingSettingsSection* renderingSettings) noexcept;
	void BuildUI(bool disableInteraction);

  private:
	enum class Section
	{
		Rendering,
	};

	void DrawNavigation();
	void DrawRenderingSection(bool disableInteraction);

	EditorRenderingSettingsSection* m_renderingSettings = nullptr;
	Section m_activeSection = Section::Rendering;
	bool m_isOpen = false;
	bool m_refreshFromRuntimeOnNextOpen = true;
};
