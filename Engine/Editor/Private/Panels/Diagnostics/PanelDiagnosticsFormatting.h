#pragma once

#include <cstdint>
#include <string>

namespace PanelDiagnosticsFormatting
{
	std::string FormatByteSize(std::uint64_t bytes);
	void DrawWrappedDisabledText(const std::string& text);
}
