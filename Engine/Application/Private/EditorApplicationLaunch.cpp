#include "PCH.h"

#include "EditorApplicationLaunch.h"

#include "Application.h"
#include "EditorApplication.h"
#include "Editor/Public/Settings/EditorSettingsBootstrap.h"
#include "Validation/RhiSmokeValidation.h"

int RunEditorApplication()
{
	return RunEditorApplication(EditorApplicationOptions{});
}

int RunEditorApplication(EditorApplicationOptions options)
{
	ApplyPersistedEditorSettingsToCVars();
	Application::ConfigureProcessFromCommandLine();

	if (RhiSmokeValidation::IsRequested())
	{
		return RhiSmokeValidation::RunEditor(std::move(options));
	}

	EditorApplication app(std::move(options));
	app.Run();
	return 0;
}
