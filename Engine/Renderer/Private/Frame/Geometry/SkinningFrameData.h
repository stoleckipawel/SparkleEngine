#pragma once

#include "Frame/Core/FrameBufferResource.h"

class RenderHardwareInterface;
struct RenderSceneData;

class SkinningFrameData final
{
  public:
	SkinningFrameData() noexcept = default;
	~SkinningFrameData() noexcept = default;

	SkinningFrameData(const SkinningFrameData&) = delete;
	SkinningFrameData& operator=(const SkinningFrameData&) = delete;
	SkinningFrameData(SkinningFrameData&& other) noexcept = default;
	SkinningFrameData& operator=(SkinningFrameData&& other) noexcept = default;

	bool IsValid() const noexcept { return m_buffer.IsValid() && m_previousBuffer.IsValid(); }
	const FrameBufferResource& GetBuffer() const noexcept { return m_buffer; }
	const FrameBufferResource& GetPreviousBuffer() const noexcept { return m_previousBuffer; }

	static SkinningFrameData Build(RenderHardwareInterface& renderHardwareInterface, const RenderSceneData& sceneData);

  private:
	FrameBufferResource m_buffer;
	FrameBufferResource m_previousBuffer;
};
