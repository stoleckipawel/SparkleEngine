#include "PCH.h"

#include "EditorApplicationLaunch.h"

#include "Application.h"
#include "EditorApplication.h"
#include "Validation/RhiSmokeValidation.h"

int RunEditorApplication()
{
	return RunEditorApplication(EditorApplicationOptions{});
}

int RunEditorApplication(EditorApplicationOptions options)
{
	Application::ConfigureProcessFromCommandLine();

	if (RhiSmokeValidation::IsRequested())
	{
		return RhiSmokeValidation::RunEditor();
	}

	EditorApplication app(std::move(options));
	app.Run();
	return 0;
}
