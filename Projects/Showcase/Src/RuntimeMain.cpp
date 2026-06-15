#include "RuntimeApplicationLaunch.h"
#include "ShowcaseSceneBehavior.h"

int main()
{
	ShowcaseSceneBehavior sceneBehavior;
	return RunRuntimeApplication(
	    RuntimeApplicationOptions{
	        .SceneUpdateCallback =
	            [&sceneBehavior](GameScene& scene, float deltaSeconds)
	            {
		            sceneBehavior.Update(scene, deltaSeconds);
	            }});
}
