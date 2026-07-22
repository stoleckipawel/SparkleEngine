#include "RenderWorldContract/RenderWorldContractChecks.h"
#include "RenderWorldContract/RenderWorldContractFixture.h"
#include "RenderWorldContract/RenderWorldContractMetrics.h"

#include <chrono>
#include <iostream>

int main()
{
	const auto buildStart = std::chrono::steady_clock::now();
	RenderWorldContractFixture fixture = BuildRenderWorldContractFixture();
	const auto buildElapsed = std::chrono::steady_clock::now() - buildStart;
	const std::size_t ownedBytes = MeasureRenderInputRecordingOwnedBytes(fixture.Recording);
	fixture.ReleaseProducerOwnership();

	RenderWorld retainedWorld;
	std::chrono::steady_clock::duration replayElapsed{};
	if (!ValidateDeterministicRenderWorldReplay(
	        fixture.Recording, fixture.StaticObject, retainedWorld, replayElapsed))
		return 1;
	if (!ValidateRenderWorldOrdering(fixture.Recording)) return 2;
	if (!ValidateRejectedRenderWorldDeltas(fixture.Recording.front(), fixture.StaticObject)) return 3;
	if (!ValidateRenderInputFrameAdmission(fixture.Recording)) return 4;

	fixture.Recording.clear();
	if (!ValidateRetainedRenderWorldOwnership(retainedWorld, fixture.StaticObject)) return 5;

	std::cout << "RenderWorld contract validation passed; logical packet bytes=" << ownedBytes
	          << ", fixture build us="
	          << std::chrono::duration_cast<std::chrono::microseconds>(buildElapsed).count()
	          << ", dual replay us="
	          << std::chrono::duration_cast<std::chrono::microseconds>(replayElapsed).count() << '\n';
	return 0;
}
