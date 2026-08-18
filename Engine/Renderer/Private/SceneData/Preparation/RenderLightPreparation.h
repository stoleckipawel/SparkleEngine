#pragma once

#include "Renderer/Public/SceneData/DirectionalLight.h"
#include "Renderer/Public/SceneData/PointLight.h"
#include "Renderer/Public/SceneData/RectLight.h"
#include "Renderer/Public/SceneData/SpotLight.h"
#include "Rendering/RenderSceneDynamicData.h"

#include <cstdint>
#include <span>

struct RenderSceneData;

enum class RenderLightClassification : std::uint8_t
{
	None,
	Directional,
	Point,
	Spot,
	Rect
};

struct PreparedRenderLight final
{
	RenderObjectId Object;
	RenderLightClassification Classification = RenderLightClassification::None;
	DirectionalLight Directional;
	PointLight Point;
	SpotLight Spot;
	RectLight Rect;
};

class RenderLightPreparation final
{
public:
	static void PrepareRange(std::span<const RenderLightData> inputs, std::span<PreparedRenderLight> outputs) noexcept;
	static void Commit(std::span<const PreparedRenderLight> lights, RenderSceneData& sceneData);

private:
	static void PrepareDirectional(
	    const SceneLightDesc& light,
	    const SceneDirectionalLightDesc& directional,
	    PreparedRenderLight& output) noexcept;
	static void PreparePoint(
	    const SceneLightDesc& light,
	    const PointLightDesc& point,
	    const DirectX::XMFLOAT3& position,
	    PreparedRenderLight& output) noexcept;
	static void PrepareSpot(
	    const SceneLightDesc& light,
	    const SpotLightDesc& spot,
	    const DirectX::XMFLOAT3& position,
	    PreparedRenderLight& output) noexcept;
	static void PrepareRect(
	    const SceneLightDesc& light,
	    const RectLightDesc& rect,
	    const DirectX::XMFLOAT3& position,
	    PreparedRenderLight& output) noexcept;
};
