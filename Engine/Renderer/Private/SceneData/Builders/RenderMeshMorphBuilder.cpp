#include "PCH.h"

#include "SceneData/Builders/RenderMeshMorphBuilder.h"

#include "GameFramework/Public/Rendering/RenderFrameDynamicData.h"
#include "Meshes/GPUMesh.h"
#include "Renderer/Public/SceneData/MeshDraw.h"
#include "SceneData/RenderSceneData.h"

#include <algorithm>

void RenderMeshMorphBuilder::Prepare(
    const RenderFrameDynamicData& dynamic,
    RenderSceneData& sceneData)
{
	m_sampledWeights.clear();
	std::size_t weightCapacity = 0u;
	for (const RenderMorphData& morph : dynamic.MorphWeights)
	{
		if (!morph.Object.IsValid() || morph.Weights.empty())
		{
			continue;
		}
		m_sampledWeights.insert_or_assign(
		    morph.Object,
		    std::span<const float>{morph.Weights});
		weightCapacity += morph.Weights.size();
	}
	for (auto& [object, history] : m_history)
	{
		history.Written = false;
		if (!m_sampledWeights.contains(object))
		{
			weightCapacity += history.Previous.size();
		}
	}
	sceneData.morphWeights.reserve(weightCapacity);
	sceneData.previousMorphWeights.reserve(weightCapacity);
}

void RenderMeshMorphBuilder::Append(
    RenderObjectId object,
    const GPUMesh& mesh,
    RenderSceneData& sceneData,
    MeshDraw& draw)
{
	const std::uint32_t targetCount =
	    mesh.GetMorphTargetCount();
	if (targetCount == 0u)
	{
		return;
	}

	const auto sampled = m_sampledWeights.find(object);
	const auto previous = m_history.find(object);
	if (sampled == m_sampledWeights.end() &&
	    (previous == m_history.end() ||
	     AreAllZero(previous->second.Previous)))
	{
		return;
	}

	const std::span<const float> sampledWeights =
	    sampled != m_sampledWeights.end()
	        ? sampled->second
	        : std::span<const float>{};
	History& history = m_history[object];
	WriteWeights(
	    sampledWeights,
	    targetCount,
	    history.Current);
	const std::vector<float>& previousWeights =
	    history.Previous.size() == history.Current.size()
	        ? history.Previous
	        : history.Current;

	draw.Morph.WeightOffset =
	    static_cast<std::uint32_t>(
	        sceneData.morphWeights.size());
	draw.Morph.TargetCount = targetCount;
	draw.Morph.VertexCount = mesh.GetVertexCount();
	sceneData.morphWeights.insert(
	    sceneData.morphWeights.end(),
	    history.Current.begin(),
	    history.Current.end());
	sceneData.previousMorphWeights.insert(
	    sceneData.previousMorphWeights.end(),
	    previousWeights.begin(),
	    previousWeights.end());
	history.Written = true;
}

void RenderMeshMorphBuilder::Commit() noexcept
{
	m_sampledWeights.clear();
	for (auto entry = m_history.begin();
	     entry != m_history.end();)
	{
		History& history = entry->second;
		if (!history.Written)
		{
			entry = m_history.erase(entry);
			continue;
		}
		history.Previous.swap(history.Current);
		history.Current.clear();
		history.Written = false;
		++entry;
	}
}

void RenderMeshMorphBuilder::Reset() noexcept
{
	m_sampledWeights.clear();
	m_history.clear();
}

bool RenderMeshMorphBuilder::AreAllZero(
    std::span<const float> weights) noexcept
{
	return std::all_of(
	    weights.begin(),
	    weights.end(),
	    [](float weight)
	    {
		    return weight == 0.0f;
	    });
}

void RenderMeshMorphBuilder::WriteWeights(
    std::span<const float> sampledWeights,
    std::uint32_t targetCount,
    std::vector<float>& outWeights)
{
	outWeights.assign(targetCount, 0.0f);
	for (std::uint32_t targetIndex = 0u;
	     targetIndex < targetCount;
	     ++targetIndex)
	{
		if (targetIndex < sampledWeights.size())
		{
			outWeights[targetIndex] =
			    sampledWeights[targetIndex];
		}
	}
}
