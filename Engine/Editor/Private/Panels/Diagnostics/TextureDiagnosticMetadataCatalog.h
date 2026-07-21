#pragma once

#include "Renderer/Public/Resources/Textures/TextureDiagnostics.h"

#include <optional>
#include <string>

struct TextureDiagnosticMetadata final
{
	std::string DisplayName;
	std::string SourcePath;
};

std::optional<TextureDiagnosticMetadata> FindTextureDiagnosticMetadata(const TextureDiagnosticsRow& row);
