#include "PCH.h"

#include "RuntimeApplicationLaunch.h"

#include "Application.h"
#include "RuntimeApplication.h"

#include "Validation/RhiSmokeValidation.h"

int RunRuntimeApplication()
{
	Application::ConfigureProcessFromCommandLine();

	if (RhiSmokeValidation::IsRequested())
	{
		return RhiSmokeValidation::RunProject();
	}

	RuntimeApplication app;
	app.Run();
	return 0;
}
