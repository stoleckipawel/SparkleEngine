#pragma once

#include <iosfwd>

namespace SparkleLauncher
{
	class LauncherShell final
	{
	  public:
		int Run(int argc, char** argv, std::ostream& output, std::ostream& error) const;
	};
}
