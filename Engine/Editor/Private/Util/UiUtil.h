#pragma once

namespace UiUtil
{
	enum class EditorIcon
	{
		None,
		Camera,
		DirectionalLight,
		StaticMesh,
		EyeVisible,
		EyeHidden,
		Reset,
		Filter,
		Settings
	};

	const char* GetEditorIconGlyph(EditorIcon icon) noexcept;
	void DrawEditorIcon(EditorIcon icon, const char* tooltip = nullptr);
	bool DrawEditorIconButton(EditorIcon icon, const char* id, const char* tooltip = nullptr);
	void DrawPlaceholderTypeIcon(const char* text, const char* tooltip = nullptr);
	bool DrawVisibilityIconButton(const char* id, bool visible);

	void DrawPanelHeader(const char* title, const char* subtitle);
	void BeginSectionCard(const char* title);
	void EndSectionCard();
	void DrawKeyValueRow(const char* label, const char* value);

	bool EditFloatSliderWithInput(
	    const char* label,
	    float& value,
	    float minValue,
	    float maxValue,
	    const char* sliderFormat,
	    const char* inputFormat);

	bool EditFloat3SliderWithInput(
	    const char* label,
	    float values[3],
	    float minValue,
	    float maxValue,
	    const char* sliderFormat,
	    const char* inputFormat);

	bool EditColor3(const char* label, float values[3]);

	bool EditCheckbox(const char* label, bool& value);

	void DrawSectionHeader(const char* title);

	bool BeginDetailsCategory(const char* title, bool defaultOpen = true);
	void EndDetailsCategory();
	void DrawDetailsValueRow(const char* label, const char* value);
	bool EditDetailsFloat(
	    const char* label,
	    float& value,
	    float speed,
	    float minValue,
	    float maxValue,
	    const char* format,
	    const float* resetValue = nullptr);
	bool EditDetailsFloat3(
	    const char* label,
	    float values[3],
	    float speed,
	    float minValue,
	    float maxValue,
	    const char* format,
	    const float* resetValues = nullptr);
	bool EditDetailsColor3(const char* label, float values[3], const float* resetValues = nullptr);
	bool EditDetailsCheckbox(const char* label, bool& value, const bool* resetValue = nullptr);
}  // namespace UiUtil