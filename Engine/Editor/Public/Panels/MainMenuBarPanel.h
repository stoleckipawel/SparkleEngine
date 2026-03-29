#pragma once

struct ImVec2;
struct ImVec4;

class LevelManager;
class Window;

class MainMenuBarPanel final
{
  public:
	MainMenuBarPanel(LevelManager* levelManager = nullptr, Window* window = nullptr) noexcept;
	~MainMenuBarPanel() = default;

	MainMenuBarPanel(const MainMenuBarPanel&) = delete;
	MainMenuBarPanel(MainMenuBarPanel&&) = delete;
	MainMenuBarPanel& operator=(const MainMenuBarPanel&) = delete;
	MainMenuBarPanel& operator=(MainMenuBarPanel&&) = delete;

	void SetLevelManager(LevelManager* levelManager) noexcept;
	void SetWindow(Window* window) noexcept;
	void BuildUI() noexcept;
	float GetHeight() const noexcept { return m_heightPixels; }

  private:
	void BuildFileMenu() noexcept;
	void BuildOpenLevelMenu() noexcept;
	void BuildWindowControls() noexcept;
	bool DrawTitleBarButton(
	    const char* id,
	    const ImVec2& size,
	    const ImVec4& baseColor,
	    const ImVec4& hoveredColor,
	    const ImVec4& activeColor) noexcept;
	void DrawMinimizeIcon() const noexcept;
	void DrawMaximizeIcon() const noexcept;
	void DrawCloseIcon() const noexcept;

	LevelManager* m_levelManager = nullptr;
	Window* m_window = nullptr;
	float m_heightPixels = 0.0f;
};
