#pragma once

class Renderer;
class RuntimeApplication;
class UI;

class EditorUiFrameRenderer final
{
  public:
	static void Render(RuntimeApplication& runtime, Renderer& renderer, UI& ui);

  private:
	EditorUiFrameRenderer() = delete;
};
