#include "EditorApp.h"
#include "UI.h"

int main()
{
	EditorApp app(CreateEditorOverlay);
	app.Run();
	return 0;
}
