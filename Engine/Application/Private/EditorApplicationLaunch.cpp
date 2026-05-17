#include "PCH.h"

#include "EditorApplicationLaunch.h"

#include "EditorApp.h"
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

int RunEditorApplication()
{
	ConfigureAutomationErrorHandling();

	if (RhiSmokeValidation::IsRequested())
	{
		return RhiSmokeValidation::RunEditor();
	}

	EditorApp app;
	app.Run();
	return 0;
}