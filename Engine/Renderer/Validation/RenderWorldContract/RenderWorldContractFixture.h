#pragma once

#include "GameFramework/Public/Rendering/RenderInputFrame.h"

#include <memory>
#include <vector>

class Mesh;

struct RenderWorldContractFixture final
{
	RenderObjectId StaticObject;
	RenderObjectId SkinnedObject;
	std::shared_ptr<const Mesh> ProducerResource;
	ImmutableRenderMeshHandle ProducerMesh;
	std::vector<RenderInputFrame> Recording;

	void ReleaseProducerOwnership() noexcept;
};

RenderWorldContractFixture BuildRenderWorldContractFixture();
RenderFrameMetadata MakeRenderWorldContractMetadata(
    std::uint64_t frameId,
    std::uint64_t sceneGeneration,
    std::uint64_t providerGeneration = 7);
