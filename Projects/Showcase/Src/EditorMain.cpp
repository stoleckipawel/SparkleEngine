#include "EditorApplicationLaunch.h"
#include "ShowcaseSceneBehavior.h"

int main()
{
	ShowcaseSceneBehavior sceneBehavior;
	return RunEditorApplication(
	    EditorApplicationOptions{
	        .RuntimeOptions =
	            RuntimeApplicationOptions{
	                .SceneUpdateCallback =
	                    [&sceneBehavior](GameScene& scene, float deltaSeconds)
	                    {
		                    sceneBehavior.Update(scene, deltaSeconds);
	                    }}});
}
