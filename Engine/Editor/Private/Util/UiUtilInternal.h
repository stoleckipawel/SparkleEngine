#pragma once

#include "Util/UiUtil.h"

struct ImDrawList;
struct ImFont;

namespace UiUtil::Internal
{
	extern const float PropertyLabelWidth;
	extern const float ScalarInputWidth;
	extern const float Float3InputWidth;
	extern const float DetailsLabelWidth;
	extern const float DetailsAxisLabelWidth;
	extern const float DetailsRowVerticalPadding;
	extern const float PlaceholderIconSize;
	extern const float DetailsUtilityColumnWidth;
	extern const float DetailsResetButtonSize;
	extern const float DetailsDirtyEpsilon;

	ImVec4 DetailsGridLineColor() noexcept;
	ImU32 DetailsRowBackgroundColor() noexcept;
	void PushFontIfAvailable(ImFont* font);
	void PopFontIfAvailable(ImFont* font);
	void DrawHeaderBar(
	    const char* title,
	    const char* trailingText,
	    float height,
	    ImU32 backgroundColor,
	    ImU32 borderColor,
	    ImFont* titleFont,
	    ImFont* trailingFont,
	    const ImVec2& padding);
	void DrawRightAlignedText(const char* value);
	ImU32 AxisColor(int axisIndex) noexcept;
	bool IsDifferentFromDefault(float value, float defaultValue) noexcept;
	bool IsDifferentFromDefault(const float values[3], const float defaultValues[3]) noexcept;
	ImVec4 WithAlpha(ImVec4 color, float alpha) noexcept;
	void DrawCenteredGlyph(ImDrawList* drawList, const ImVec2& start, const ImVec2& size, const char* glyph, ImU32 color) noexcept;
	bool BeginDetailsRow(const char* label, int valueColumnCount);
	bool DrawDetailsResetButton(int utilityColumnIndex, bool dirty, const char* tooltip = "Reset to default");
	void DrawDetailsEmptyUtility(int utilityColumnIndex);
	void EndDetailsRow();
}
