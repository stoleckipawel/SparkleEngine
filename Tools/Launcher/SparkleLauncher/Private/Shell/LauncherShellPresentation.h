#pragma once

#include <iosfwd>

namespace SparkleLauncher
{
	struct LauncherShellModel;

	void RenderLauncherShell(const LauncherShellModel& model, std::ostream& output);
}
