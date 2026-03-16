#pragma once

namespace UiUtil
{
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

	void DrawSectionHeader(const char* title);
}