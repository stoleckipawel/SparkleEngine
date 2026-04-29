#pragma once

struct ImFont;

namespace SparkleUiTheme
{
	void ApplyEditorialDarkTheme();
	void ConfigureTypography(float dpiScale);
	bool AreEditorIconsAvailable() noexcept;
	ImFont* GetBodyFont();
	ImFont* GetHeadingFont();
	ImFont* GetMonoFont();
}  // namespace SparkleUiTheme