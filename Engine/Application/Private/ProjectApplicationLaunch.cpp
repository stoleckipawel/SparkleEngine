#include "PCH.h"

#include "ProjectApplicationLaunch.h"

#include "ApplicationBase.h"
#include "ProjectApp.h"

#include "Validation/RhiSmokeValidation.h"

int RunProjectApplication()
{
	ApplicationBase::ConfigureProcessFromCommandLine();

	if (RhiSmokeValidation::IsRequested())
	{
		return RhiSmokeValidation::RunProject();
	}

	ProjectApp app;
	app.Run();
	return 0;
}