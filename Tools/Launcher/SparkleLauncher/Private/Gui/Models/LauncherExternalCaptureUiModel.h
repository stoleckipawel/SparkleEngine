#pragma once

#include "LauncherContextUiModel.h"
#include <string_view>
namespace SparkleLauncher
{
	QVector<LauncherSelectionOption> BuildExternalCaptureOptions(std::string_view api, std::string_view profile);
}
