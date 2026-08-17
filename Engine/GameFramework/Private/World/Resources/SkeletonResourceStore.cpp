#include "PCH.h"

#include "World/Resources/SkeletonResourceStore.h"

#include "GameFramework/Public/Scene/Animations/SkeletonTransformContract.h"

class SkeletonTransformTranslation final
{
public:
	static ECS::AnimationJointTransform DecomposeLocalTransform(const SkeletonResource& skeleton, std::size_t jointIndex) noexcept
	{
		const DirectX::XMMATRIX local = DirectX::XMLoadFloat4x4(&skeleton.joints[jointIndex].bindLocalTransform);
		DirectX::XMVECTOR scale;
		DirectX::XMVECTOR rotation;
		DirectX::XMVECTOR translation;
		ECS::AnimationJointTransform result;
		if (DirectX::XMMatrixDecompose(&scale, &rotation, &translation, local))
		{
			DirectX::XMStoreFloat3(&result.Translation, translation);
			DirectX::XMStoreFloat4(&result.Rotation, DirectX::XMQuaternionNormalize(rotation));
			DirectX::XMStoreFloat3(&result.Scale, scale);
		}
		return result;
	}
};

void SkeletonResourceStore::Append(std::vector<SkeletonResource>&& skeletons)
{
	m_entries.reserve(m_entries.size() + skeletons.size());
	for (SkeletonResource& skeleton : skeletons)
	{
		if (skeleton.assetId == Assets::InvalidCookedAssetId || m_byAssetId.contains(skeleton.assetId))
			continue;
		Entry entry;
		entry.BindLocalTransforms.reserve(skeleton.joints.size());
		for (std::size_t jointIndex = 0; jointIndex < skeleton.joints.size(); ++jointIndex)
			entry.BindLocalTransforms.push_back(SkeletonTransformTranslation::DecomposeLocalTransform(skeleton, jointIndex));
		if (!SkeletonTransformContract::BuildEvaluationOrder(skeleton.joints, entry.EvaluationOrder))
		{
			continue;
		}
		entry.Resource = std::move(skeleton);
		const auto slot = static_cast<std::uint32_t>(m_entries.size());
		m_entries.push_back(std::move(entry));
		m_byAssetId.emplace(m_entries.back().Resource.assetId, SkeletonResourceHandle{slot, m_entries.back().Generation});
	}
}

SkeletonResourceHandle SkeletonResourceStore::Find(Assets::CookedAssetId skeletonAssetId) const noexcept
{
	const auto found = m_byAssetId.find(skeletonAssetId);
	return found == m_byAssetId.end() ? SkeletonResourceHandle{} : found->second;
}

ECS::SkeletonEvaluationData SkeletonResourceStore::Resolve(SkeletonResourceHandle handle) const noexcept
{
	if (!handle.IsValid() || handle.Slot >= m_entries.size())
		return {};
	const Entry& entry = m_entries[handle.Slot];
	return entry.Generation == handle.Generation
	    ? ECS::SkeletonEvaluationData{&entry.Resource, entry.BindLocalTransforms, entry.EvaluationOrder}
	    : ECS::SkeletonEvaluationData{};
}
