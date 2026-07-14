#pragma once

#include "Frame/Core/FrameBufferResource.h"

class RenderHardwareInterface;
struct RenderSceneData;

class MeshInstanceFrameData final
{
  public:
	MeshInstanceFrameData() noexcept = default;
	~MeshInstanceFrameData() noexcept = default;

	MeshInstanceFrameData(const MeshInstanceFrameData&) = delete;
	MeshInstanceFrameData& operator=(const MeshInstanceFrameData&) = delete;
	MeshInstanceFrameData(MeshInstanceFrameData&& other) noexcept = default;
	MeshInstanceFrameData& operator=(MeshInstanceFrameData&& other) noexcept = default;

	bool IsValid() const noexcept { return m_buffer.IsValid(); }
	const FrameBufferResource& GetBuffer() const noexcept { return m_buffer; }

	static MeshInstanceFrameData Build(RenderHardwareInterface& renderHardwareInterface, const RenderSceneData& sceneData);

  private:
	FrameBufferResource m_buffer;
};
