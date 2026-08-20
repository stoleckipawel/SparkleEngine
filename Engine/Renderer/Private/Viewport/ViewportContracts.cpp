#include "PCH.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

RenderOutputFlags operator|(RenderOutputFlags lhs, RenderOutputFlags rhs) noexcept
{
	return static_cast<RenderOutputFlags>(static_cast<std::uint16_t>(lhs) | static_cast<std::uint16_t>(rhs));
}

RenderOutputFlags operator&(RenderOutputFlags lhs, RenderOutputFlags rhs) noexcept
{
	return static_cast<RenderOutputFlags>(static_cast<std::uint16_t>(lhs) & static_cast<std::uint16_t>(rhs));
}

RenderOutputFlags& operator|=(RenderOutputFlags& lhs, RenderOutputFlags rhs) noexcept
{
	lhs = lhs | rhs;
	return lhs;
}

bool HasAnyRenderOutputFlags(RenderOutputFlags flags, RenderOutputFlags test) noexcept
{
	return (flags & test) != RenderOutputFlags::None;
}

bool RenderViewportExtent::IsValid() const noexcept
{
	return Width > 0 && Height > 0;
}

bool RenderViewportExtent::operator==(const RenderViewportExtent& other) const noexcept
{
	return Width == other.Width && Height == other.Height;
}

RenderViewSelectionToken::operator bool() const noexcept
{
	return Value != 0;
}

RenderProductHandle::operator bool() const noexcept
{
	return Value != 0;
}

ViewportCaptureResult::operator bool() const noexcept
{
	return Status == ViewportCaptureStatus::Succeeded;
}

ViewportCaptureId::operator bool() const noexcept
{
	return Value != 0;
}

bool ViewportRenderProducts::HasOutput(RenderOutputFlags output) const noexcept
{
	return HasAnyRenderOutputFlags(m_availableOutputs, output);
}

const RenderProduct* ViewportRenderProducts::FindProduct(RenderOutputFlags output) const noexcept
{
	const RenderProduct* product = SelectProduct(output);
	return product != nullptr && HasOutput(output) ? product : nullptr;
}

void ViewportRenderProducts::Clear() noexcept
{
	m_availableOutputs = RenderOutputFlags::None;
	m_sceneColor = {};
	m_sceneDepth = {};
	m_objectId = {};
	m_normals = {};
	m_overlayMask = {};
}

void ViewportRenderProducts::ClearProduct(RenderOutputFlags output) noexcept
{
	RenderProduct* product = SelectProduct(output);
	if (product == nullptr)
	{
		return;
	}

	*product = {};
	RemoveAvailableOutput(output);
}

void ViewportRenderProducts::SetProduct(RenderOutputFlags output, RenderProduct product) noexcept
{
	RenderProduct* target = SelectProduct(output);
	if (target == nullptr)
	{
		return;
	}

	*target = product;
	if (product.Handle)
	{
		m_availableOutputs |= output;
		return;
	}

	RemoveAvailableOutput(output);
}

RenderProduct* ViewportRenderProducts::SelectProduct(RenderOutputFlags output) noexcept
{
	switch (output)
	{
		case RenderOutputFlags::SceneColor:
			return &m_sceneColor;
		case RenderOutputFlags::SceneDepth:
			return &m_sceneDepth;
		case RenderOutputFlags::ObjectId:
			return &m_objectId;
		case RenderOutputFlags::Normals:
			return &m_normals;
		case RenderOutputFlags::OverlayMask:
			return &m_overlayMask;
		case RenderOutputFlags::None:
		default:
			return nullptr;
	}
}

const RenderProduct* ViewportRenderProducts::SelectProduct(RenderOutputFlags output) const noexcept
{
	switch (output)
	{
		case RenderOutputFlags::SceneColor:
			return &m_sceneColor;
		case RenderOutputFlags::SceneDepth:
			return &m_sceneDepth;
		case RenderOutputFlags::ObjectId:
			return &m_objectId;
		case RenderOutputFlags::Normals:
			return &m_normals;
		case RenderOutputFlags::OverlayMask:
			return &m_overlayMask;
		case RenderOutputFlags::None:
		default:
			return nullptr;
	}
}

void ViewportRenderProducts::RemoveAvailableOutput(RenderOutputFlags output) noexcept
{
	m_availableOutputs =
	    static_cast<RenderOutputFlags>(static_cast<std::uint16_t>(m_availableOutputs) & ~static_cast<std::uint16_t>(output));
}
