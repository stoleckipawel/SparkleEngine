#include "PCH.h"

#include "Application.h"

#include "ApplicationCommandLineCVars.h"
#include "Core/Public/Diagnostics/Trace.h"
#include "Core/Public/Environment/EnvironmentVariables.h"

#include <cstdlib>

void Application::ConfigureProcessFromCommandLine() noexcept
{
	if (Environment::GetFlag("SPARKLE_SUPPRESS_CRASH_DIALOGS"))
	{
		SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
		_set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
	}

	ApplicationCommandLineCVars::Apply();
}

void Application::Run()
{
	Diagnostics::BeginTraceSession();

	Initialize();

	while (Tick())
	{
	}

	Shutdown();

	Diagnostics::EndTraceSession();
}