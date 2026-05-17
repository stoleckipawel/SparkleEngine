#include "PCH.h"

#include "ProjectApplicationLaunch.h"

#include "ProjectApp.h"

#include "Validation/RhiSmokeValidation.h"

#include "Core/Public/Environment/EnvironmentVariables.h"

#include <cstdlib>

namespace
{
	void ConfigureAutomationErrorHandling() noexcept
	{
		if (!Environment::GetFlag("SPARKLE_SUPPRESS_CRASH_DIALOGS"))
		{
			return;
		}

		SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
		_set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
	}
}

int RunProjectApplication()
{
	ConfigureAutomationErrorHandling();

	if (RhiSmokeValidation::IsRequested())
	{
		return RhiSmokeValidation::RunProject();
	}

	ProjectApp app;
	app.Run();
	return 0;
}