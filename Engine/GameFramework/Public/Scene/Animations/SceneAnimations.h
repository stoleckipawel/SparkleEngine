#pragma once

#include "GameFramework/Public/Scene/Animations/SceneAnimation.h"
#include "GameFramework/Public/GameFrameworkAPI.h"

#include <cstddef>
#include <vector>

class SPARKLE_ENGINE_API SceneAnimations final
{
  public:
	void Clear() noexcept;
	void AppendClips(std::vector<SceneAnimationClipDesc>&& clips);

	std::size_t GetClipCount() const noexcept { return m_clips.size(); }
	const std::vector<SceneAnimationClipDesc>& GetClips() const noexcept { return m_clips; }

  private:
	std::vector<SceneAnimationClipDesc> m_clips;
};
