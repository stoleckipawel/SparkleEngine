#include "PCH.h"

#include "EditorApplicationLaunch.h"

#include "Application.h"
#include "EditorApplication.h"
#include "Validation/RhiSmokeValidation.h"

int RunEditorApplication()
{
	Application::ConfigureProcessFromCommandLine();

	if (RhiSmokeValidation::IsRequested())
	{
		return RhiSmokeValidation::RunEditor();
	}

	EditorApplication app;
	app.Run();
	return 0;
}
