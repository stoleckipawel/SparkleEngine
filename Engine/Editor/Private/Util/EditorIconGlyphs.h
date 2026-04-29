#pragma once

namespace EditorIconGlyphs
{
	// Font Awesome Free Solid 6.7.1 glyphs used by Sparkle's semantic editor icon layer.
	// Keep this file intentionally small so the editor depends only on the font asset,
	// not a generated third-party icon header.
	namespace FontAwesome
	{
		inline constexpr const char* Folder = "\xef\x81\xbb";
		inline constexpr const char* Camera = "\xef\x80\xb0";
		inline constexpr const char* DirectionalLight = "\xef\x86\x85";
		inline constexpr const char* StaticMesh = "\xef\x86\xb2";
		inline constexpr const char* EyeVisible = "\xef\x81\xae";
		inline constexpr const char* EyeHidden = "\xef\x81\xb0";
		inline constexpr const char* Reset = "\xef\x83\xa2";
		inline constexpr const char* Filter = "\xef\x82\xb0";
		inline constexpr const char* Settings = "\xef\x80\x93";
	}

	namespace Fallback
	{
		inline constexpr const char* Folder = "+";
		inline constexpr const char* Camera = "C";
		inline constexpr const char* DirectionalLight = "L";
		inline constexpr const char* StaticMesh = "M";
		inline constexpr const char* EyeVisible = "o";
		inline constexpr const char* EyeHidden = "-";
		inline constexpr const char* Reset = "r";
		inline constexpr const char* Filter = "F";
		inline constexpr const char* Settings = "S";
	}
}  // namespace EditorIconGlyphs
