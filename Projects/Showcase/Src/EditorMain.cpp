#include "EditorApplicationLaunch.h"
#include "ShowcaseSceneController.h"
#include "GameFramework/Public/Scene/GameScene.h"

#include <memory>

int main()
{
	return RunEditorApplication(
	    RuntimeApplicationOptions{
	        .SceneSetupCallback =
	            [](GameScene& scene)
	            {
		            scene.RegisterController(std::make_unique<ShowcaseSceneController>());
	            }});
}
