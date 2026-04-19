#include "PCH.h"
#include "Panels/ViewportPanel.h"

#include "Util/UiUtil.h"

#include <algorithm>

#include <imgui.h>

namespace
{
	constexpr float MinimumViewportExtent = 64.0f;

	const char* ToViewKindLabel(RenderViewKind viewKind) noexcept
	{
		switch (viewKind)
		{
			case RenderViewKind::Game:
				return "Game";
			case RenderViewKind::Scene:
				return "Scene";
			case RenderViewKind::Preview:
				return "Preview";
			case RenderViewKind::Thumbnail:
				return "Thumbnail";
			case RenderViewKind::Debug:
				return "Debug";
			default:
				return "Viewport";
		}
	}

	ImVec2 ComputeViewportImageSize(const ImVec2& availableRegion, const RenderViewportExtent& extent) noexcept
	{
		if (!extent.IsValid() || availableRegion.x <= 0.0f || availableRegion.y <= 0.0f)
		{
			return ImVec2(0.0f, 0.0f);
		}

		const float extentWidth = static_cast<float>(extent.Width);
		const float extentHeight = static_cast<float>(extent.Height);
		const float scale = (std::min) (availableRegion.x / extentWidth, availableRegion.y / extentHeight);
		return ImVec2(extentWidth * scale, extentHeight * scale);
	}
}

ViewportPanel::ViewportPanel(float leftInsetPixels, float rightInsetPixels) noexcept :
    m_leftInsetPixels(leftInsetPixels), m_rightInsetPixels(rightInsetPixels)
{
	m_renderRequest.ViewportId = 1;
	m_renderRequest.ViewKind = RenderViewKind::Game;
	m_renderRequest.RequestedOutputs = RenderOutputFlags::SceneColor | RenderOutputFlags::SceneDepth;
	m_renderRequest.Extent = RenderViewportExtent{1280u, 720u};
}

void ViewportPanel::SetTopInset(float topInsetPixels) noexcept
{
	m_topInsetPixels = topInsetPixels;
}

void ViewportPanel::SetSideInsets(float leftInsetPixels, float rightInsetPixels) noexcept
{
	m_leftInsetPixels = leftInsetPixels;
	m_rightInsetPixels = rightInsetPixels;
}

void ViewportPanel::SetRequestedExtent(RenderViewportExtent extent) noexcept
{
	if (extent.IsValid())
	{
		m_renderRequest.Extent = extent;
	}
}

void ViewportPanel::SetRenderProducts(const ViewportRenderProducts& renderProducts) noexcept
{
	m_renderProducts = renderProducts;
}

void ViewportPanel::SetSceneColorTextureId(std::uint64_t textureId) noexcept
{
	m_sceneColorTextureId = textureId;
}

const ViewportRenderRequest& ViewportPanel::GetRenderRequest() const noexcept
{
	return m_renderRequest;
}

void ViewportPanel::UpdateRequestedExtent(float availableWidth, float availableHeight) noexcept
{
	const float clampedWidth = (std::max) (MinimumViewportExtent, availableWidth);
	const float clampedHeight = (std::max) (MinimumViewportExtent, availableHeight);
	m_renderRequest.Extent = RenderViewportExtent{static_cast<std::uint32_t>(clampedWidth), static_cast<std::uint32_t>(clampedHeight)};
}

void ViewportPanel::BuildEmptyState() noexcept
{
	ImGui::TextDisabled("Viewport output unavailable");
	ImGui::Spacing();
	ImGui::TextWrapped("EditorApp is requesting runtime scene output, but no scene color surface is available for presentation yet.");
}

void ViewportPanel::BuildUI(bool disableInteraction)
{
	ImGuiIO& io = ImGui::GetIO();
	const float width = (std::max) (MinimumViewportExtent, io.DisplaySize.x - m_leftInsetPixels - m_rightInsetPixels);
	const float height = (std::max) (MinimumViewportExtent, io.DisplaySize.y - m_topInsetPixels);

	ImGui::SetNextWindowPos(ImVec2(m_leftInsetPixels, m_topInsetPixels), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin(
	    "Viewport",
	    nullptr,
	    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
	ImGui::PopStyleVar(); // WindowPadding

	const float surfaceRegionHeight = (std::max) (MinimumViewportExtent, ImGui::GetContentRegionAvail().y);
	ImGui::BeginDisabled(disableInteraction);
	ImGui::BeginChild(
	    "##ViewportSurface",
	    ImVec2(0.0f, surfaceRegionHeight),
	    ImGuiChildFlags_None,
	    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

	const ImVec2 availableRegion = ImGui::GetContentRegionAvail();
	UpdateRequestedExtent(availableRegion.x, availableRegion.y);

	if (m_sceneColorTextureId == 0 || !m_renderProducts.SceneColor.Handle)
	{
		BuildEmptyState();
	}
	else
	{
		const RenderProduct& sceneColor = m_renderProducts.SceneColor;
		const ImVec2 imageSize = ComputeViewportImageSize(availableRegion, sceneColor.Extent);
		const ImVec2 start = ImGui::GetCursorPos();
		if (availableRegion.x > imageSize.x)
		{
			ImGui::SetCursorPosX(start.x + ((availableRegion.x - imageSize.x) * 0.5f));
		}
		if (availableRegion.y > imageSize.y)
		{
			ImGui::SetCursorPosY(start.y + ((availableRegion.y - imageSize.y) * 0.5f));
		}

		ImGui::Image(static_cast<ImTextureID>(m_sceneColorTextureId), imageSize);
	}

	ImGui::EndChild();
	ImGui::EndDisabled();

	ImGui::End();
}