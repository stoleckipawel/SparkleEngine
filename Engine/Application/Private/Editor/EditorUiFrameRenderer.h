#pragma once

class Renderer;
class UI;

class EditorUiFrameRenderer final
{
public:
	static void Render(Renderer& renderer, UI& ui);

private:
	EditorUiFrameRenderer() = delete;
};
