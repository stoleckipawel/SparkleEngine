#include "PCH.h"

#include "ApplicationLaunch.h"

#include "EditorApp.h"
#include "ProjectApp.h"

#include "Validation/D3D12SmokeValidation.h"

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

class ApplicationLaunch final
{
  public:
	static int RunProject() noexcept;
	static int RunEditor() noexcept;
};

int ApplicationLaunch::RunProject() noexcept
{
	ConfigureAutomationErrorHandling();

	if (D3D12SmokeValidation::IsRequested())
	{
		return D3D12SmokeValidation::RunProject();
	}

	ProjectApp app;
	app.Run();
	return 0;
}

int ApplicationLaunch::RunEditor() noexcept
{
	ConfigureAutomationErrorHandling();

	if (D3D12SmokeValidation::IsRequested())
	{
		return D3D12SmokeValidation::RunEditor();
	}

	EditorApp app;
	app.Run();
	return 0;
}

int RunProjectApplication()
{
	return ApplicationLaunch::RunProject();
}

int RunEditorApplication()
{
	return ApplicationLaunch::RunEditor();
}