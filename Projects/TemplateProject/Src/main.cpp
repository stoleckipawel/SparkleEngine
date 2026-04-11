#include "App.h"
#include "UI.h"

int main()
{
	App app(CreateEditorOverlay);
	app.Run();
	return 0;
}
