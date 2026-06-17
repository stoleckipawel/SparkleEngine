#pragma once

class EngineRenderingSettingsSection;

class RenderingSettingsPanel final
{
  public:
	void SetSettings(EngineRenderingSettingsSection* settings) noexcept;
	void RefreshFromRuntimeState() noexcept;
	void BuildUI(bool disableInteraction);

  private:
	EngineRenderingSettingsSection* m_settings = nullptr;
};
