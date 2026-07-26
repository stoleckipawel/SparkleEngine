#pragma once

#include "Rendering/RenderObjectId.h"

#include <cstdint>
#include <map>
#include <span>
#include <vector>

class GPUMesh;
struct MeshDraw;
struct RenderFrameDynamicData;
struct RenderSceneData;

// Owns per-object morph history and packs current/previous weights into one frame's scene data.
class RenderMeshMorphBuilder final
{
  public:
	void Prepare(
	    const RenderFrameDynamicData& dynamic,
	    RenderSceneData& sceneData);
	void Append(
	    RenderObjectId object,
	    const GPUMesh& mesh,
	    RenderSceneData& sceneData,
	    MeshDraw& draw);
	void Commit() noexcept;
	void Reset() noexcept;

  private:
	static bool AreAllZero(
	    std::span<const float> weights) noexcept;
	static void WriteWeights(
	    std::span<const float> sampledWeights,
	    std::uint32_t targetCount,
	    std::vector<float>& outWeights);

	struct History final
	{
		std::vector<float> Previous;
		std::vector<float> Current;
		bool Written = false;
	};

	std::map<RenderObjectId, std::span<const float>> m_sampledWeights;
	std::map<RenderObjectId, History> m_history;
};
