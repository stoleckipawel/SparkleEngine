#pragma once

struct ImFont;

namespace SparkleUiTheme
{
	void ApplyEditorialDarkTheme();
	void ConfigureTypography(float dpiScale);
	ImFont* GetBodyFont();
	ImFont* GetHeadingFont();
	ImFont* GetMonoFont();
}