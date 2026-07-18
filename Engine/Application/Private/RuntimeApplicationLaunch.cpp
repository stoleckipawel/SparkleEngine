#include "PCH.h"

#include "RuntimeApplicationLaunch.h"

#include "Application.h"
#include "RuntimeApplication.h"
#include "Core/Public/Threading/ThreadOwnership.h"

int RunRuntimeApplication()
{
	return RunRuntimeApplication(RuntimeApplicationOptions{});
}

int RunRuntimeApplication(RuntimeApplicationOptions options)
{
	Threading::SetCurrentThreadRole("Sparkle.GameThread");
	Application::ConfigureProcessFromCommandLine();

	RuntimeApplication app(std::move(options));
	app.Run();
	return 0;
}
