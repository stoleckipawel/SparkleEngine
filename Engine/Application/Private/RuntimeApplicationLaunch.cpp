#include "PCH.h"

#include "RuntimeApplicationLaunch.h"

#include "Application.h"
#include "RuntimeApplication.h"

#include "Validation/RhiSmokeValidation.h"

int RunRuntimeApplication()
{
	return RunRuntimeApplication(RuntimeApplicationOptions{});
}

int RunRuntimeApplication(RuntimeApplicationOptions options)
{
	Application::ConfigureProcessFromCommandLine();

	if (RhiSmokeValidation::IsRequested())
	{
		return RhiSmokeValidation::RunProject(std::move(options));
	}

	RuntimeApplication app(std::move(options));
	app.Run();
	return 0;
}
