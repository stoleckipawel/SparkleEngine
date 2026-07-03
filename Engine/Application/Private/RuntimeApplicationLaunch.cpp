#include "PCH.h"

#include "RuntimeApplicationLaunch.h"

#include "Application.h"
#include "RuntimeApplication.h"

int RunRuntimeApplication()
{
	return RunRuntimeApplication(RuntimeApplicationOptions{});
}

int RunRuntimeApplication(RuntimeApplicationOptions options)
{
	Application::ConfigureProcessFromCommandLine();

	RuntimeApplication app(std::move(options));
	app.Run();
	return 0;
}
