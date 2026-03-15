#pragma once

#include "SceneData/RenderSceneSnapshot.h"

class Scene;

class RenderSceneSnapshotCache final
{
  public:
	RenderSceneSnapshotCache() = default;
	~RenderSceneSnapshotCache() noexcept = default;

	RenderSceneSnapshotCache(const RenderSceneSnapshotCache&) = delete;
	RenderSceneSnapshotCache& operator=(const RenderSceneSnapshotCache&) = delete;
	RenderSceneSnapshotCache(RenderSceneSnapshotCache&&) = delete;
	RenderSceneSnapshotCache& operator=(RenderSceneSnapshotCache&&) = delete;

	const RenderSceneSnapshot& Capture(const Scene& scene);
	void Reset() noexcept;

  private:
	RenderSceneSnapshot m_snapshot;
};