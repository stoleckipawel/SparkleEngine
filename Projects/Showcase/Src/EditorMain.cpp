#include "EditorApplicationLaunch.h"
#include "ShowcaseSceneController.h"

#include <memory>

int main()
{
	return RunEditorApplication(
	    EditorApplicationOptions{
	        .RuntimeOptions =
	            RuntimeApplicationOptions{
	                .SceneSetupCallback =
	                    [](GameScene& scene)
	                    {
		                    scene.RegisterController(std::make_unique<ShowcaseSceneController>());
	                    }}});
}
