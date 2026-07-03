#pragma once

#include <string>

#include <imgui.h>

namespace UiUtil
{
	enum class EditorIcon
	{
		None,
		Folder,
		FolderOpen,
		Camera,
		Light,
		DirectionalLight,
		PointLight,
		SpotLight,
		StaticMesh,
		Material,
		EyeVisible,
		EyeHidden,
		Reset,
		Filter,
		Settings,
		Save,
		Shader,
		Refresh,
		Reload,
		Search,
		Level,
		ViewMode,
		ViewLit,
		ViewDiffuse,
		ViewNormal,
		ViewRoughness,
		ViewMetallic,
		ViewEmissive,
		ViewAmbientOcclusion,
		ViewSubsurfaceColor,
		ViewSubsurfaceStrength,
		ViewDirectDiffuse,
		ViewDirectSpecular,
		ViewDirectSubsurface,
		Cpu,
		Gpu,
		Help,
		Clear,
		Copy,
		Console,
		SourceFile,
		Reflection,
		Disassembly,
		CompileRequest,
		Sort
	};

	const char* GetEditorIconGlyph(EditorIcon icon) noexcept;
	std::string MakeIconLabel(EditorIcon icon, const char* label);
	bool MatchesDetailsFilter(const std::string& filterText, const char* title, const char* keywords) noexcept;
	ImU32 WithAlphaU32(ImVec4 color, float alpha) noexcept;
	void DrawEditorIcon(EditorIcon icon, const char* tooltip = nullptr, bool drawBadgeBackground = true);
	bool DrawEditorIconButton(EditorIcon icon, const char* id, const char* tooltip = nullptr);
	void DrawPlaceholderTypeIcon(const char* text, const char* tooltip = nullptr, bool drawBadgeBackground = true);
	bool DrawVisibilityIconButton(const char* id, bool visible);
	bool DrawFilterChip(const char* label, bool active) noexcept;
	void DrawMutedText(const char* text, float alpha = 0.72f) noexcept;
	bool DrawCenteredVisibilityIconButton(const char* id, bool visible) noexcept;

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
	void DrawDetailsEmptyState(const char* text = "Select an object from the scene outliner to inspect its properties.");
	void DrawDetailsValueRow(const char* label, const char* value);
	void DrawDetailsAssetRow(const char* label, EditorIcon thumbnailIcon, const char* value, const char* typeText = nullptr);
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
