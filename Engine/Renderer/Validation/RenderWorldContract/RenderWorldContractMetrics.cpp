#include "RenderWorldContract/RenderWorldContractMetrics.h"

std::size_t MeasureRenderInputOwnedBytes(const RenderInputFrame& frame) noexcept
{
	std::size_t bytes = sizeof(frame) + frame.WorldDelta.Creates.size() * sizeof(RenderObjectCreate) +
	                    frame.WorldDelta.Updates.size() * sizeof(RenderObjectUpdate) +
	                    frame.WorldDelta.Destroys.size() * sizeof(RenderObjectId) +
	                    frame.WorldDelta.InstanceGroups.Values.size() * sizeof(RenderMeshInstanceGroupData) +
	                    frame.Dynamic.Objects.size() * sizeof(RenderObjectDynamicData) +
	                    frame.Dynamic.Lights.size() * sizeof(RenderLightData) +
	                    frame.Dynamic.Skinning.size() * sizeof(RenderSkinningData) +
	                    frame.Dynamic.MorphWeights.size() * sizeof(RenderMorphData);
	if (frame.WorldDelta.Materials)
		bytes += frame.WorldDelta.Materials->Values.size() * sizeof(MaterialDesc);
	if (frame.WorldDelta.Textures)
		for (const RenderTextureAsset& asset : frame.WorldDelta.Textures->Assets)
			bytes += asset.Path.native().size() * sizeof(std::filesystem::path::value_type);
	for (const RenderSkinningData& row : frame.Dynamic.Skinning)
		bytes += row.Matrices.size() * sizeof(DirectX::XMFLOAT4X4);
	for (const RenderMorphData& row : frame.Dynamic.MorphWeights) bytes += row.Weights.size() * sizeof(float);
	return bytes;
}

std::size_t MeasureRenderInputRecordingOwnedBytes(std::span<const RenderInputFrame> recording) noexcept
{
	std::size_t bytes = 0;
	for (const RenderInputFrame& frame : recording) bytes += MeasureRenderInputOwnedBytes(frame);
	return bytes;
}
